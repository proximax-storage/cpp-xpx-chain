/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "catapult/cache/ReadOnlyCatapultCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/BalanceView.h"
#include "catapult/chain/ChainResults.h"
#include "catapult/consumers/BlockChainProcessor.h"
#include "catapult/consumers/InputUtils.h"
#include "catapult/model/BlockUtils.h"
#include "test/MockSyncState.h"
#include "tests/catapult/consumers/test/ConsumerTestUtils.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/nodeps/ParamsCapture.h"
#include "tests/TestHarness.h"

using namespace catapult::validators;
using catapult::disruptor::BlockElements;

namespace catapult { namespace consumers {

#define TEST_CLASS BlockChainProcessorTests

	namespace {
		// region MockCommitBlock

		struct MockCommitBlockParams {
		public:
			MockCommitBlockParams(const model::BlockElement& block, const observers::ObserverState& state)
					: Block(&block)
					, State(state)
			{}

		public:
			const model::BlockElement* Block;
			const observers::ObserverState State;
		};

		class MockCommitBlock : public test::ParamsCapture<MockCommitBlockParams> {
		public:
			MockCommitBlock() : m_numCalls(0), m_trigger(std::numeric_limits<size_t>::max())
			{}

		public:
			bool operator()(const model::BlockElement& block, const observers::ObserverState& state) const {
				const_cast<MockCommitBlock*>(this)->push(block, state);
				return ++m_numCalls < m_trigger;
			}

		public:
			void setFailure(size_t trigger) {
				m_trigger = trigger;
			}

		private:
			mutable size_t m_numCalls;
			size_t m_trigger;
		};

		// endregion

		// region MockBlockHitPredicate

		struct BlockHitPredicateParams {
		public:
			BlockHitPredicateParams(const Hash256& hash, const BlockTarget& target, const utils::TimeSpan& time, const Amount& balance)
					: Hash(hash)
					, Target(target)
					, Timestamp(time)
					, Balance(balance)
			{}

		public:
			Hash256 Hash;
			BlockTarget Target;
			utils::TimeSpan Timestamp;
			Amount Balance;
		};

		class MockBlockHitPredicate : public test::ParamsCapture<BlockHitPredicateParams> {
		public:
			MockBlockHitPredicate() : m_numCalls(0), m_trigger(std::numeric_limits<size_t>::max())
			{}

		public:
			bool operator()(const Hash256& hash, const BlockTarget& target, const utils::TimeSpan& time, const Amount& balance) const {
				const_cast<MockBlockHitPredicate*>(this)->push(hash, target, time, balance);
				return ++m_numCalls < m_trigger;
			}

		public:
			void setFailure(size_t trigger) {
				m_trigger = trigger;
			}

		private:
			mutable size_t m_numCalls;
			size_t m_trigger;
		};

		// endregion

		// region MockBatchEntityProcessor

		struct BatchEntityProcessorParams {
		public:
			BatchEntityProcessorParams(
					catapult::Height height,
					catapult::Timestamp timestamp,
					const model::WeakEntityInfos& entityInfos,
					const observers::ObserverState& state)
					: Height(height)
					, Timestamp(timestamp)
					, EntityInfos(entityInfos)
					, State(state)
			{}

		public:
			const catapult::Height Height;
			const catapult::Timestamp Timestamp;
			const model::WeakEntityInfos EntityInfos;
			const observers::ObserverState State;
		};

		class MockBatchEntityProcessor : public test::ParamsCapture<BatchEntityProcessorParams> {
		public:
			MockBatchEntityProcessor() : m_numCalls(0), m_trigger(0), m_result(ValidationResult::Success)
			{}

		public:
			ValidationResult operator()(
					const Height& height,
					const Timestamp& timestamp,
					const model::WeakEntityInfos& entityInfos,
					const observers::ObserverState& state) const {
				const_cast<MockBatchEntityProcessor*>(this)->push(height, timestamp, entityInfos, state);
				return ++m_numCalls < m_trigger ? ValidationResult::Success : m_result;
			}

		public:
			void setResult(ValidationResult result, size_t trigger) {
				m_result = result;
				m_trigger = trigger;
			}

		private:
			mutable size_t m_numCalls;
			size_t m_trigger;
			ValidationResult m_result;
		};

		// endregion

		model::WeakEntityInfos ExtractEntityInfosFromBlock(const model::BlockElement& element) {
			model::WeakEntityInfos entityInfos;
			ExtractEntityInfos(element, entityInfos);
			return entityInfos;
		}

		struct ProcessorTestContext {
		public:
			ProcessorTestContext() {
				Handlres.BatchEntityProcessor = [this](const auto& height, const auto& timestamp, const auto& entities, const auto& state) {
					return BatchEntityProcessor(height, timestamp, entities, state);
				};
				Handlres.CommitBlock = [this](const model::BlockElement& block, const observers::ObserverState& state) {
					return CommitBlock(block, state);
				};
				Processor = CreateBlockChainProcessor(
						[this]() {
							return [this](const auto& hash, const auto& target, const auto& time, const auto& balance) {
								return BlockHitPredicate(hash, target, time, balance);
							};
						},
						Handlres
				);
			}

		public:
			MockBlockHitPredicate BlockHitPredicate;
			MockBatchEntityProcessor BatchEntityProcessor;
			MockCommitBlock CommitBlock;
			BlockChainSyncHandlers Handlres;
			BlockChainProcessor Processor;

		public:
			ValidationResult Process(BlockElements& elements) {
				SyncState state;
				return Processor(state, elements);
			}

			ValidationResult Process(SyncState& state, BlockElements& elements) {
				return Processor(state, elements);
			}

		public:
			void assertBlockHitPredicateCalls(SyncState& state, const BlockElements& elements) {
				// block hit
				{
					const auto& allParams = BlockHitPredicate.params();
					ASSERT_LE(allParams.size(), elements.size());

					auto i = 0u;
					auto previousTimeStamp = Timestamp(0);
					for (auto params : allParams) {
						auto message = "hit predicate at " + std::to_string(i);
						const auto& currentBlockElement = elements[i];

						auto expectedEffectiveBalance = cache::BalanceView(
								cache::ReadOnlyAccountStateCache(state.currentObserverState().Cache.sub<cache::AccountStateCache>()),
								cache::ReadOnlyAccountStateCache(state.preivousObserverState().Cache.sub<cache::AccountStateCache>()),
								state.EffectiveBalanceHeight
						).getEffectiveBalance(currentBlockElement.Block.Signer, currentBlockElement.Block.Height - Height(1));

						EXPECT_EQ(utils::TimeSpan::FromDifference(currentBlockElement.Block.Timestamp, previousTimeStamp), params.Timestamp) << message;
						EXPECT_EQ(currentBlockElement.Block.BaseTarget, params.Target) << message;
						EXPECT_EQ(currentBlockElement.GenerationHash, params.Hash) << message;
						EXPECT_EQ(expectedEffectiveBalance, params.Balance) << message;
						++i;
						previousTimeStamp = currentBlockElement.Block.Timestamp;
					}
				}
			}

			void assertBatchEntityProcessorCalls(SyncState& state, const BlockElements& elements) {
				const auto& allParams = BatchEntityProcessor.params();
				ASSERT_LE(allParams.size(), elements.size());

				auto i = 0u;
				for (auto params : allParams) {
					auto message = "processor at " + std::to_string(i);
					const auto& blockElement = elements[i];
					auto expectedEntityInfos = ExtractEntityInfosFromBlock(blockElement);

					EXPECT_EQ(blockElement.Block.Height, params.Height) << message;
					EXPECT_EQ(blockElement.Block.Timestamp, params.Timestamp) << message;
					EXPECT_EQ(expectedEntityInfos, params.EntityInfos) << message;
					EXPECT_EQ(&state.currentObserverState().Cache, &params.State.Cache) << message;
					++i;
				}
			}

			void assertCommitBlockCalls(SyncState& state, const BlockElements& elements) {
				const auto& allParams = CommitBlock.params();
				ASSERT_LE(allParams.size(), elements.size());

				auto i = 0u;
				for (auto params : allParams) {
					auto message = "commit block at " + std::to_string(i);
					const auto& blockElement = elements[i];

					EXPECT_EQ(&blockElement, params.Block) << message;
					EXPECT_EQ(&state.preivousObserverState().Cache, &params.State.Cache) << message;
					++i;
				}
			}

			void assertNoHandlerCalls() {
				EXPECT_EQ(0u, BlockHitPredicate.params().size());
				EXPECT_EQ(0u, BatchEntityProcessor.params().size());
				EXPECT_EQ(0u, CommitBlock.params().size());
			}
		};
	}

	TEST(TEST_CLASS, EmptyInputResultsInNeutralResult) {
		// Arrange:
		ProcessorTestContext context;
		auto pParentBlock = test::GenerateEmptyRandomBlock();
		BlockElements elements;

		// Act:
		auto result = context.Process(elements);

		// Assert:
		EXPECT_EQ(ValidationResult::Neutral, result);
		context.assertNoHandlerCalls();
	}

	namespace {
		void AssertUnlinkedChain(const consumer<model::Block&>& unlink) {
			ProcessorTestContext context;
			auto pParentBlock = test::GenerateEmptyRandomBlock();
			pParentBlock->Height = Height(11);
			auto elements = test::CreateBlockElements(1);
			test::LinkBlocks(*pParentBlock, const_cast<model::Block&>(elements[0].Block));
			unlink(const_cast<model::Block&>(elements[0].Block));
			std::unique_ptr<test::MockSyncState> state = std::make_unique<test::MockSyncState>(
					test::CreateCatapultCacheWithMarkerAccount(),
					test::CreateCatapultCacheWithMarkerAccount(),
					test::BlockToBlockElement(*pParentBlock)
			);

			// Act:
			auto result = context.Process(*state, elements);

			// Assert:
			EXPECT_EQ(chain::Failure_Chain_Unlinked, result);
			context.assertNoHandlerCalls();
		}
	}

	TEST(TEST_CLASS, ChainPartHeightMustLinkToParent) {
		// Assert: invalidate the height
		AssertUnlinkedChain([](auto& block) { block.Height = block.Height + Height(1); });
	}

	TEST(TEST_CLASS, ChainPartPreviousBlockHashMustLinkToParent) {
		// Assert: invalidate the previous block hash
		AssertUnlinkedChain([](auto& block) { ++block.PreviousBlockHash[0]; });
	}

	namespace {
		void AssertCanProcessValidElements(BlockElements& elements, size_t numExpectedBlocks) {
			ProcessorTestContext context;
			auto pParentBlock = test::GenerateEmptyRandomBlock();
			pParentBlock->Height = Height(11);
			test::LinkBlocks(*pParentBlock, const_cast<model::Block&>(elements[0].Block));
			std::unique_ptr<test::MockSyncState> state = std::make_unique<test::MockSyncState>(
					test::CreateCatapultCacheWithMarkerAccount(),
					test::CreateCatapultCacheWithMarkerAccount(),
					test::BlockToBlockElement(*pParentBlock)
			);

			EXPECT_EQ(state->EffectiveBalanceHeight, Height(0));

			// Act:
			auto result = context.Process(*state, elements);

			// Assert:
			EXPECT_EQ(ValidationResult::Success, result);
			EXPECT_EQ(numExpectedBlocks, context.BlockHitPredicate.params().size());
			EXPECT_EQ(numExpectedBlocks, context.BatchEntityProcessor.params().size());
			EXPECT_EQ(numExpectedBlocks, context.CommitBlock.params().size());
			context.assertBlockHitPredicateCalls(*state, elements);
			context.assertBatchEntityProcessorCalls(*state, elements);
			context.assertCommitBlockCalls(*state, elements);
		}
	}

	TEST(TEST_CLASS, CanProcessSingleBlockWithoutTransactions) {
		// Arrange:
		auto elements = test::CreateBlockElements(1);

		// Assert:
		AssertCanProcessValidElements(elements, 1);
	}

	TEST(TEST_CLASS, CanProcessSingleBlockWithTransactions) {
		// Arrange:
		auto pBlock = test::GenerateBlockWithTransactionsAtHeight(3, 12);
		auto elements = test::CreateBlockElements({ pBlock.get() });

		// Assert:
		AssertCanProcessValidElements(elements, 1);
	}

	TEST(TEST_CLASS, CanProcessMultipleBlocksWithoutTransactionss) {
		// Arrange:
		auto elements = test::CreateBlockElements(3);

		// Assert:
		AssertCanProcessValidElements(elements, 3);
	}

	TEST(TEST_CLASS, CanProcessMultipleBlocksWithTransactions) {
		// Arrange:
		auto pBlock1 = test::GenerateBlockWithTransactionsAtHeight(3, 12);
		auto pBlock2 = test::GenerateBlockWithTransactionsAtHeight(2, 13);
		auto pBlock3 = test::GenerateBlockWithTransactionsAtHeight(4, 14);
		auto elements = test::CreateBlockElements({ pBlock1.get(), pBlock2.get(), pBlock3.get() });

		// Assert:
		AssertCanProcessValidElements(elements, 3);
	}

	TEST(TEST_CLASS, ExecuteShortCircutsOnUnhitBlock) {
		// Arrange: cause the second hit check to return false
		ProcessorTestContext context;
		context.BlockHitPredicate.setFailure(2);

		auto pParentBlock = test::GenerateEmptyRandomBlock();
		pParentBlock->Height = Height(11);
		auto elements = test::CreateBlockElements(3);
		test::LinkBlocks(*pParentBlock, const_cast<model::Block&>(elements[0].Block));
		std::unique_ptr<test::MockSyncState> state = std::make_unique<test::MockSyncState>(
				test::CreateCatapultCacheWithMarkerAccount(),
				test::CreateCatapultCacheWithMarkerAccount(),
				test::BlockToBlockElement(*pParentBlock)
		);

		EXPECT_EQ(state->EffectiveBalanceHeight, Height(0));

		// Act:
		auto result = context.Process(*state, elements);

		// Assert:
		// - block hit predicate returned { true, false }
		// - only one processor was called (after true, but not false)
		EXPECT_EQ(chain::Failure_Chain_Block_Not_Hit, result);
		EXPECT_EQ(2u, context.BlockHitPredicate.params().size());
		EXPECT_EQ(1u, context.BatchEntityProcessor.params().size());
		EXPECT_EQ(1u, context.CommitBlock.params().size());
		context.assertBlockHitPredicateCalls(*state, elements);
		context.assertBatchEntityProcessorCalls(*state, elements);
		context.assertCommitBlockCalls(*state, elements);
	}

	namespace {
		void AssertShortCircutsOnProcessorResult(ValidationResult processorResult) {
			// Arrange: cause the second processor call to return a non-success code
			ProcessorTestContext context;
			context.BatchEntityProcessor.setResult(processorResult, 2);

			auto pParentBlock = test::GenerateEmptyRandomBlock();
			pParentBlock->Height = Height(11);
			auto elements = test::CreateBlockElements(3);
			test::LinkBlocks(*pParentBlock, const_cast<model::Block&>(elements[0].Block));
			std::unique_ptr<test::MockSyncState> state = std::make_unique<test::MockSyncState>(
					test::CreateCatapultCacheWithMarkerAccount(),
					test::CreateCatapultCacheWithMarkerAccount(),
					test::BlockToBlockElement(*pParentBlock)
			);

			EXPECT_EQ(state->EffectiveBalanceHeight, Height(0));

			// Act:
			auto result = context.Process(*state, elements);

			// Assert:
			// - block hit predicate returned true
			// - the second processor returned a non-success code
			EXPECT_EQ(processorResult, result);
			EXPECT_EQ(2u, context.BlockHitPredicate.params().size());
			EXPECT_EQ(2u, context.BatchEntityProcessor.params().size());
			EXPECT_EQ(1u, context.CommitBlock.params().size());
			context.assertBlockHitPredicateCalls(*state, elements);
			context.assertBatchEntityProcessorCalls(*state, elements);
			context.assertCommitBlockCalls(*state, elements);
		}
	}

	TEST(TEST_CLASS, ExecuteShortCircutsOnProcessorResult_Neutral) {
		// Assert:
		AssertShortCircutsOnProcessorResult(ValidationResult::Neutral);
	}

	TEST(TEST_CLASS, ExecuteShortCircutsOnProcessorResult_Failure) {
		// Assert:
		AssertShortCircutsOnProcessorResult(ValidationResult::Failure);
	}

	// region generation hashes update

	namespace {
		void AssertGenerationHashesAreUpdatedCorrectly(BlockElements& elements, size_t numExpectedBlocks) {
			// Arrange:
			ProcessorTestContext context;
			auto pParentBlock = test::GenerateEmptyRandomBlock();
			pParentBlock->Height = Height(11);
			test::LinkBlocks(*pParentBlock, const_cast<model::Block&>(elements[0].Block));

			// - clear all generation hashes
			for (auto& blockElement : elements)
				blockElement.GenerationHash = {};

			// Act:
			auto parentBlockElement = test::BlockToBlockElement(*pParentBlock);
			test::FillWithRandomData(parentBlockElement.GenerationHash);
			std::unique_ptr<test::MockSyncState> state = std::make_unique<test::MockSyncState>(
					test::CreateCatapultCacheWithMarkerAccount(),
					test::CreateCatapultCacheWithMarkerAccount(),
					parentBlockElement
			);
			// Act:
			auto result = context.Process(*state, elements);

			// Sanity:
			EXPECT_EQ(ValidationResult::Success, result);
			EXPECT_EQ(numExpectedBlocks, elements.size());

			// Assert: generation hashes were calculated
			auto i = 0u;
			auto previousGenerationHash = parentBlockElement.GenerationHash;
			for (const auto& blockElement : elements) {
				auto expectedGenerationHash = model::CalculateGenerationHash(previousGenerationHash, blockElement.Block.Signer);
				EXPECT_EQ(expectedGenerationHash, blockElement.GenerationHash) << "generation hash at " << i++;
				previousGenerationHash = expectedGenerationHash;
			}
		}
	}

	TEST(TEST_CLASS, SetsGenerationHashesInSingleBlockInput) {
		// Arrange:
		auto elements = test::CreateBlockElements(1);

		// Assert:
		AssertGenerationHashesAreUpdatedCorrectly(elements, 1);
	}

	TEST(TEST_CLASS, SetsGenerationHashesInMultiBlockInput) {
		// Arrange:
		auto elements = test::CreateBlockElements(3);

		// Assert:
		AssertGenerationHashesAreUpdatedCorrectly(elements, 3);
	}

	// endregion
}}
