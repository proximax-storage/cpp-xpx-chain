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

#include "catapult/cache_core/BlockDifficultyCache.h"
#include "catapult/chain/ChainResults.h"
#include "catapult/model/BlockUtils.h"
#include "tests/catapult/consumers/test/ConsumerTestUtils.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/local/ServiceLocatorTestContext.h"

using namespace catapult::validators;
using catapult::disruptor::BlockElements;

namespace catapult { namespace consumers {

#define TEST_CLASS BlockChainProcessorTests

	namespace {
		// region MockBlockHitPredicate

		struct BlockHitPredicateParams {
		public:
			BlockHitPredicateParams(
					const model::Block* pParent,
					const model::Block* pChild,
					const catapult::GenerationHash& generationHash)
					: pParentBlock(pParent)
					, pChildBlock(pChild)
					, GenerationHash(generationHash)
			{}

		public:
			const model::Block* pParentBlock;
			const model::Block* pChildBlock;
			const catapult::GenerationHash GenerationHash;
		};

		class MockBlockHitPredicate : public test::ParamsCapture<BlockHitPredicateParams> {
		public:
			MockBlockHitPredicate() : m_numCalls(0), m_trigger(std::numeric_limits<size_t>::max())
			{}

		public:
			bool operator()(const model::Block& parent, const model::Block& child, const GenerationHash& generationHash) const {
				const_cast<MockBlockHitPredicate*>(this)->push(&parent, &child, generationHash);
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

		// region MockBlockHitPredicateFactory

		struct BlockHitPredicateFactoryParams {
		public:
			BlockHitPredicateFactoryParams(const cache::ReadOnlyCatapultCache& cache)
					: IsPassedMarkedCache(test::IsMarkedCache(cache))
					, NumDifficultyInfos(cache.sub<cache::BlockDifficultyCache>().size())
			{}

		public:
			const bool IsPassedMarkedCache;
			const size_t NumDifficultyInfos;
		};

		class MockBlockHitPredicateFactory : public test::ParamsCapture<BlockHitPredicateFactoryParams> {
		public:
			MockBlockHitPredicateFactory(const MockBlockHitPredicate& blockHitPredicate)
					: m_blockHitPredicate(blockHitPredicate)
			{}

		public:
			BlockHitPredicate operator()(const cache::ReadOnlyCatapultCache& cache) const {
				const_cast<MockBlockHitPredicateFactory*>(this)->push(cache);

				const auto& blockHitPredicate = m_blockHitPredicate;
				return [&blockHitPredicate](const auto& parent, const auto& child, const auto& generationHash) {
					return blockHitPredicate(parent, child, generationHash);
				};
			}

		private:
			const MockBlockHitPredicate& m_blockHitPredicate;
		};

		// endregion

		// region MockBatchEntityProcessor

		struct BatchEntityProcessorParams {
		public:
			BatchEntityProcessorParams(
					catapult::Height height,
					catapult::Timestamp timestamp,
					const model::WeakEntityInfos& entityInfos,
					observers::ObserverState& state)
					: Height(height)
					, Timestamp(timestamp)
					, EntityInfos(entityInfos)
					, pState(&state.State)
					, IsPassedMarkedCache(test::IsMarkedCache(state.Cache))
					, NumDifficultyInfos(state.Cache.sub<cache::BlockDifficultyCache>().size())
			{}

		public:
			const catapult::Height Height;
			const catapult::Timestamp Timestamp;
			const model::WeakEntityInfos EntityInfos;
			const state::CatapultState* pState;
			const bool IsPassedMarkedCache;
			const size_t NumDifficultyInfos;
		};

		void AddHeightReceipt(model::BlockStatementBuilder& blockStatementBuilder, Height height) {
			model::Receipt receipt{};
			receipt.Size = sizeof(model::Receipt);
			receipt.Type = static_cast<model::ReceiptType>(height.unwrap());

			blockStatementBuilder.addTransactionReceipt(receipt);
		}

		class MockBatchEntityProcessor : public test::ParamsCapture<BatchEntityProcessorParams> {
		public:
			MockBatchEntityProcessor() : m_numCalls(0), m_trigger(0), m_result(ValidationResult::Success)
			{}

		public:
			ValidationResult operator()(
					Height height,
					Timestamp timestamp,
					const model::WeakEntityInfos& entityInfos,
					observers::ObserverState& state) const {
				const_cast<MockBatchEntityProcessor*>(this)->push(height, timestamp, entityInfos, state);

				// - add a block difficulty info to the cache as a marker
				auto& blockDifficultyCache = state.Cache.sub<cache::BlockDifficultyCache>();
				blockDifficultyCache.insert(state::BlockDifficultyInfo(Height(blockDifficultyCache.size() + 1)));

				// - add a receipt as a marker
				if (state.pBlockStatementBuilder)
					AddHeightReceipt(*state.pBlockStatementBuilder, height);

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

		// region ProcessorTestContext

		struct ProcessorTestContext {
		public:
			explicit ProcessorTestContext(ReceiptValidationMode receiptValidationMode = ReceiptValidationMode::Disabled)
					: BlockHitPredicateFactory(BlockHitPredicate) {
				auto& config = const_cast<config::ImmutableConfiguration&>(ServiceState.state().config().Immutable);
				config.ShouldEnableVerifiableReceipts = (receiptValidationMode == ReceiptValidationMode::Enabled);
				Processor = CreateBlockChainProcessor(
						[this](const auto& cache) {
							return BlockHitPredicateFactory(cache);
						},
						[this](auto height, auto timestamp, const auto& entities, auto& state) {
							return BatchEntityProcessor(height, timestamp, entities, state);
						},
						ServiceState.state());
			}

		public:
			state::CatapultState State;

			MockBlockHitPredicate BlockHitPredicate;
			MockBlockHitPredicateFactory BlockHitPredicateFactory;
			MockBatchEntityProcessor BatchEntityProcessor;
			BlockChainProcessor Processor;
			test::ServiceTestState ServiceState;

		public:
			ValidationResult Process(const model::BlockElement& parentBlockElement, BlockElements& elements) {
				auto cache = test::CreateCatapultCacheWithMarkerAccount();
				auto delta = cache.createDelta();
				std::vector<std::unique_ptr<model::Notification>> notifications;
				auto observerState = observers::ObserverState(delta, State, notifications);

				return Processor(WeakBlockInfo(parentBlockElement), elements, observerState);
			}

			ValidationResult Process(const model::Block& parentBlock, BlockElements& elements) {
				// use the real parent block hash to ensure the chain is linked
				return Process(test::BlockToBlockElement(parentBlock), elements);
			}

		public:
			void assertBlockHitPredicateCalls(const model::Block& parentBlock, const BlockElements& elements) {
				// block hit predicate factory
				{
					const auto& allParams = BlockHitPredicateFactory.params();
					EXPECT_EQ(1u, allParams.size());

					auto i = 0u;
					for (auto params : allParams) {
						auto message = "hit predicate factory at " + std::to_string(i);
						EXPECT_TRUE(params.IsPassedMarkedCache) << message;
						EXPECT_EQ(i, params.NumDifficultyInfos) << message;
						++i;
					}
				}

				// block hit
				{
					const auto& allParams = BlockHitPredicate.params();
					ASSERT_LE(allParams.size(), elements.size());

					auto i = 0u;
					const auto* pPreviousBlock = &parentBlock;
					for (auto params : allParams) {
						auto message = "hit predicate at " + std::to_string(i);
						const auto& currentBlockElement = elements[i];
						const auto* pCurrentBlock = &currentBlockElement.Block;
						EXPECT_EQ(pPreviousBlock, params.pParentBlock) << message;
						EXPECT_EQ(pCurrentBlock, params.pChildBlock) << message;
						EXPECT_EQ(currentBlockElement.GenerationHash, params.GenerationHash) << message;
						pPreviousBlock = pCurrentBlock;
						++i;
					}
				}
			}

			void assertBatchEntityProcessorCalls(const BlockElements& elements) {
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
					EXPECT_EQ(&State, params.pState) << message;
					EXPECT_TRUE(params.IsPassedMarkedCache) << message;
					EXPECT_EQ(i, params.NumDifficultyInfos) << message;
					++i;
				}
			}

			void assertNoHandlerCalls() {
				EXPECT_EQ(0u, BlockHitPredicate.params().size());
				EXPECT_EQ(0u, BlockHitPredicateFactory.params().size());
				EXPECT_EQ(0u, BatchEntityProcessor.params().size());
			}
		};

		// endregion

		void ClearStateHashes(BlockElements& elements) {
			for (auto& blockElement : elements)
				const_cast<model::Block&>(blockElement.Block).StateHash = Hash256();
		}

		void ClearReceiptsHashes(BlockElements& elements) {
			for (auto& blockElement : elements)
				const_cast<model::Block&>(blockElement.Block).BlockReceiptsHash = Hash256();
		}

		void PrepareChain(Height height, model::Block& parentBlock, BlockElements& elements) {
			parentBlock.Height = height;
			test::LinkBlocks(parentBlock, const_cast<model::Block&>(elements[0].Block));
			ClearStateHashes(elements);
			ClearReceiptsHashes(elements);
		}
	}

	// region neutral - empty

	TEST(TEST_CLASS, EmptyInputResultsInNeutralResult) {
		// Arrange:
		ProcessorTestContext context;
		auto pParentBlock = test::GenerateEmptyRandomBlock();
		BlockElements elements;

		// Act:
		auto result = context.Process(*pParentBlock, elements);

		// Assert:
		EXPECT_EQ(ValidationResult::Neutral, result);
		context.assertNoHandlerCalls();
	}

	// endregion

	// region valid

	namespace {
		struct ReceiptValidationDisabledTraits {
			static constexpr auto Mode = ReceiptValidationMode::Disabled;

			static void PrepareReceiptsHashes(BlockElements&)
			{}
		};

		struct ReceiptValidationEnabledTraits {
			static constexpr auto Mode = ReceiptValidationMode::Enabled;

			static Hash256 GetExpectedHash(Height height) {
				model::BlockStatementBuilder blockStatementBuilder;
				AddHeightReceipt(blockStatementBuilder, height);
				return CalculateMerkleHash(*blockStatementBuilder.build());
			}

			static void PrepareReceiptsHashes(BlockElements& elements) {
				// Arrange: one "height" receipt is added per block
				for (auto& blockElement : elements)
					const_cast<model::Block&>(blockElement.Block).BlockReceiptsHash = GetExpectedHash(blockElement.Block.Height);
			}
		};

		template<typename TTraits>
		void AssertCanProcessValidElements(BlockElements& elements, size_t numExpectedBlocks) {
			ProcessorTestContext context(TTraits::Mode);
			auto pParentBlock = test::GenerateEmptyRandomBlock();
			PrepareChain(Height(11), *pParentBlock, elements);
			TTraits::PrepareReceiptsHashes(elements);

			// Act:
			auto result = context.Process(*pParentBlock, elements);

			// Assert:
			EXPECT_EQ(ValidationResult::Success, result);
			EXPECT_EQ(numExpectedBlocks, context.BlockHitPredicate.params().size());
			EXPECT_EQ(numExpectedBlocks, context.BatchEntityProcessor.params().size());
			context.assertBlockHitPredicateCalls(*pParentBlock, elements);
			context.assertBatchEntityProcessorCalls(elements);
		}
	}

#define RECEIPT_VALIDATION_MODE_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_ReceiptDisabled) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ReceiptValidationDisabledTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_ReceiptEnabled) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ReceiptValidationEnabledTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	RECEIPT_VALIDATION_MODE_BASED_TEST(CanProcessSingleBlockWithoutTransactions) {
		// Arrange:
		auto elements = test::CreateBlockElements(1);

		// Assert:
		AssertCanProcessValidElements<TTraits>(elements, 1);
	}

	RECEIPT_VALIDATION_MODE_BASED_TEST(CanProcessSingleBlockWithTransactions) {
		// Arrange:
		auto pBlock = test::GenerateBlockWithTransactions(3, Height(12));
		auto elements = test::CreateBlockElements({ pBlock.get() });

		// Assert:
		AssertCanProcessValidElements<TTraits>(elements, 1);
	}

	RECEIPT_VALIDATION_MODE_BASED_TEST(CanProcessMultipleBlocksWithoutTransactionss) {
		// Arrange:
		auto elements = test::CreateBlockElements(3);

		// Assert:
		AssertCanProcessValidElements<TTraits>(elements, 3);
	}

	RECEIPT_VALIDATION_MODE_BASED_TEST(CanProcessMultipleBlocksWithTransactions) {
		// Arrange:
		auto pBlock1 = test::GenerateBlockWithTransactions(3, Height(12));
		auto pBlock2 = test::GenerateBlockWithTransactions(2, Height(13));
		auto pBlock3 = test::GenerateBlockWithTransactions(4, Height(14));
		auto elements = test::CreateBlockElements({ pBlock1.get(), pBlock2.get(), pBlock3.get() });

		// Assert:
		AssertCanProcessValidElements<TTraits>(elements, 3);
	}

	// endregion

	// region invalid - unlinked

	namespace {
		void AssertUnlinkedChain(const consumer<model::Block&>& unlink) {
			ProcessorTestContext context;
			auto pParentBlock = test::GenerateEmptyRandomBlock();
			auto elements = test::CreateBlockElements(1);
			PrepareChain(Height(11), *pParentBlock, elements);
			unlink(const_cast<model::Block&>(elements[0].Block));

			// Act:
			auto result = context.Process(*pParentBlock, elements);

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

	// endregion

	// region invalid - unhit

	TEST(TEST_CLASS, ExecuteShortCircutsOnUnhitBlock) {
		// Arrange: cause the second hit check to return false
		ProcessorTestContext context;
		context.BlockHitPredicate.setFailure(2);

		auto pParentBlock = test::GenerateEmptyRandomBlock();
		auto elements = test::CreateBlockElements(3);
		PrepareChain(Height(11), *pParentBlock, elements);

		// Act:
		auto result = context.Process(*pParentBlock, elements);

		// Assert:
		// - block hit predicate returned { true, false }
		// - only one processor was called (after true, but not false)
		EXPECT_EQ(chain::Failure_Chain_Block_Not_Hit, result);
		EXPECT_EQ(2u, context.BlockHitPredicate.params().size());
		EXPECT_EQ(1u, context.BatchEntityProcessor.params().size());
		context.assertBlockHitPredicateCalls(*pParentBlock, elements);
		context.assertBatchEntityProcessorCalls(elements);
	}

	// endregion

	// region invalid - processor

	namespace {
		void AssertShortCircutsOnProcessorResult(ValidationResult processorResult) {
			// Arrange: cause the second processor call to return a non-success code
			ProcessorTestContext context;
			context.BatchEntityProcessor.setResult(processorResult, 2);

			auto pParentBlock = test::GenerateEmptyRandomBlock();
			auto elements = test::CreateBlockElements(3);
			PrepareChain(Height(11), *pParentBlock, elements);

			// Act:
			auto result = context.Process(*pParentBlock, elements);

			// Assert:
			// - block hit predicate returned true
			// - the second processor returned a non-success code
			EXPECT_EQ(processorResult, result);
			EXPECT_EQ(2u, context.BlockHitPredicate.params().size());
			EXPECT_EQ(2u, context.BatchEntityProcessor.params().size());
			context.assertBlockHitPredicateCalls(*pParentBlock, elements);
			context.assertBatchEntityProcessorCalls(elements);
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

	// endregion

	// region invalid - state hash

	TEST(TEST_CLASS, ExecuteShortCircutsOnInconsistentStateHash) {
		// Arrange:
		ProcessorTestContext context;
		auto pParentBlock = test::GenerateEmptyRandomBlock();
		auto elements = test::CreateBlockElements(3);
		PrepareChain(Height(11), *pParentBlock, elements);

		// - invalidate the second block state hash
		test::FillWithRandomData(const_cast<model::Block&>(elements[1].Block).StateHash);

		// Act:
		auto result = context.Process(*pParentBlock, elements);

		// Assert:
		// - block hit predicate returned true
		// - processor returned success
		EXPECT_EQ(chain::Failure_Chain_Block_Inconsistent_State_Hash, result);
		EXPECT_EQ(2u, context.BlockHitPredicate.params().size());
		EXPECT_EQ(2u, context.BatchEntityProcessor.params().size());
		context.assertBlockHitPredicateCalls(*pParentBlock, elements);
		context.assertBatchEntityProcessorCalls(elements);
	}

	// endregion

	// region invalid - block receipts hash

	TEST(TEST_CLASS, ExecuteShortCircutsOnInconsistentBlockReceiptsHash) {
		// Arrange:
		ProcessorTestContext context(ReceiptValidationMode::Enabled);
		auto pParentBlock = test::GenerateEmptyRandomBlock();
		auto elements = test::CreateBlockElements(3);
		PrepareChain(Height(11), *pParentBlock, elements);
		ReceiptValidationEnabledTraits::PrepareReceiptsHashes(elements);

		// - invalidate the second block receipts hash
		test::FillWithRandomData(const_cast<model::Block&>(elements[1].Block).BlockReceiptsHash);

		// Act:
		auto result = context.Process(*pParentBlock, elements);

		// Assert:
		// - block hit predicate returned true
		// - processor returned success
		EXPECT_EQ(chain::Failure_Chain_Block_Inconsistent_Receipts_Hash, result);
		EXPECT_EQ(2u, context.BlockHitPredicate.params().size());
		EXPECT_EQ(2u, context.BatchEntityProcessor.params().size());
		context.assertBlockHitPredicateCalls(*pParentBlock, elements);
		context.assertBatchEntityProcessorCalls(elements);
	}

	// endregion

	// region update - generation hashes

	namespace {
		void AssertGenerationHashesAreUpdatedCorrectly(size_t numExpectedBlocks) {
			// Arrange:
			ProcessorTestContext context;
			auto pParentBlock = test::GenerateEmptyRandomBlock();
			auto elements = test::CreateBlockElements(numExpectedBlocks);
			PrepareChain(Height(11), *pParentBlock, elements);

			// - clear all generation hashes
			for (auto& blockElement : elements)
				blockElement.GenerationHash = {};

			// Act:
			auto parentBlockElement = test::BlockToBlockElement(*pParentBlock);
			test::FillWithRandomData(parentBlockElement.GenerationHash);
			auto result = context.Process(parentBlockElement, elements);

			// Sanity:
			EXPECT_EQ(ValidationResult::Success, result);
			EXPECT_EQ(numExpectedBlocks, elements.size());

			// Assert: generation hashes were calculated
			auto i = 0u;
			auto previousGenerationHash = parentBlockElement.GenerationHash;
			for (const auto& blockElement : elements) {
				auto expectedGenerationHash = model::CalculateGenerationHash(previousGenerationHash, blockElement.Block.Signer);
				EXPECT_EQ(expectedGenerationHash, blockElement.GenerationHash) << "generation hash at " << i;
				previousGenerationHash = expectedGenerationHash;
				++i;
			}
		}
	}

	TEST(TEST_CLASS, SetsGenerationHashesInSingleBlockInput) {
		// Assert:
		AssertGenerationHashesAreUpdatedCorrectly(1);
	}

	TEST(TEST_CLASS, SetsGenerationHashesInMultiBlockInput) {
		// Assert:
		AssertGenerationHashesAreUpdatedCorrectly(3);
	}

	// endregion

	// region update - sub cache merkle roots

	namespace {
		void AssertSubCacheMerkleRootsAreUpdatedCorrectly(size_t numExpectedBlocks) {
			// Arrange:
			ProcessorTestContext context;
			auto pParentBlock = test::GenerateEmptyRandomBlock();
			auto elements = test::CreateBlockElements(numExpectedBlocks);
			PrepareChain(Height(11), *pParentBlock, elements);

			// - set all sub cache merkle roots
			for (auto& blockElement : elements)
				blockElement.SubCacheMerkleRoots = test::GenerateRandomDataVector<Hash256>(3);

			// Act:
			auto parentBlockElement = test::BlockToBlockElement(*pParentBlock);
			auto result = context.Process(parentBlockElement, elements);

			// Sanity:
			EXPECT_EQ(ValidationResult::Success, result);
			EXPECT_EQ(numExpectedBlocks, elements.size());

			// Assert: sub cache merkle roots were set (cleared)
			auto i = 0u;
			for (const auto& blockElement : elements) {
				EXPECT_TRUE(blockElement.SubCacheMerkleRoots.empty()) << "sub cache merkle roots at " << i;
				++i;
			}
		}
	}

	TEST(TEST_CLASS, SetsSubCacheMerkleRootsInSingleBlockInput) {
		// Assert:
		AssertSubCacheMerkleRootsAreUpdatedCorrectly(1);
	}

	TEST(TEST_CLASS, SetsSubCacheMerkleRootsInMultiBlockInput) {
		// Assert:
		AssertSubCacheMerkleRootsAreUpdatedCorrectly(3);
	}

	// endregion

	// region update - block statements

	namespace {
		void AssertBlockStatementsAreNotSetWhenReceiptValidationIsDisabled(size_t numExpectedBlocks) {
			// Arrange:
			ProcessorTestContext context;
			auto pParentBlock = test::GenerateEmptyRandomBlock();
			auto elements = test::CreateBlockElements(numExpectedBlocks);
			PrepareChain(Height(11), *pParentBlock, elements);

			// Act:
			auto parentBlockElement = test::BlockToBlockElement(*pParentBlock);
			auto result = context.Process(parentBlockElement, elements);

			// Sanity:
			EXPECT_EQ(ValidationResult::Success, result);
			EXPECT_EQ(numExpectedBlocks, elements.size());

			// Assert: block statements were not set
			auto i = 0u;
			for (const auto& blockElement : elements) {
				auto message = "block statement at " + std::to_string(i);
				EXPECT_FALSE(!!blockElement.OptionalStatement) << message;
				++i;
			}
		}
	}

	TEST(TEST_CLASS, DoesNotSetBlockStatementsInSingleBlockInputWhenReceiptValidationIsDisabled) {
		// Assert:
		AssertBlockStatementsAreNotSetWhenReceiptValidationIsDisabled(1);
	}

	TEST(TEST_CLASS, DoesNotSetBlockStatementsInMultiBlockInputWhenReceiptValidationIsDisabled) {
		// Assert:
		AssertBlockStatementsAreNotSetWhenReceiptValidationIsDisabled(3);
	}

	namespace {
		void AssertBlockStatementsAreUpdatedCorrectlyWhenReceiptValidationIsEnabled(size_t numExpectedBlocks) {
			// Arrange:
			ProcessorTestContext context(ReceiptValidationMode::Enabled);
			auto pParentBlock = test::GenerateEmptyRandomBlock();
			auto elements = test::CreateBlockElements(numExpectedBlocks);
			PrepareChain(Height(11), *pParentBlock, elements);
			ReceiptValidationEnabledTraits::PrepareReceiptsHashes(elements);

			// Act:
			auto parentBlockElement = test::BlockToBlockElement(*pParentBlock);
			auto result = context.Process(parentBlockElement, elements);

			// Sanity:
			EXPECT_EQ(ValidationResult::Success, result);
			EXPECT_EQ(numExpectedBlocks, elements.size());

			// Assert: block statements were set
			auto i = 0u;
			for (const auto& blockElement : elements) {
				auto message = "block statement at " + std::to_string(i);
				ASSERT_TRUE(!!blockElement.OptionalStatement) << message;
				auto expectedBlockStatementHash = ReceiptValidationEnabledTraits::GetExpectedHash(blockElement.Block.Height);
				auto blockStatementHash = CalculateMerkleHash(*blockElement.OptionalStatement);
				EXPECT_EQ(expectedBlockStatementHash, blockStatementHash) << message;
				++i;
			}
		}
	}

	TEST(TEST_CLASS, SetsBlockStatementsInSingleBlockInputWhenReceiptValidationIsEnabled) {
		// Assert:
		AssertBlockStatementsAreUpdatedCorrectlyWhenReceiptValidationIsEnabled(1);
	}

	TEST(TEST_CLASS, SetsBlockStatementsInMultiBlockInputWhenReceiptValidationIsEnabled) {
		// Assert:
		AssertBlockStatementsAreUpdatedCorrectlyWhenReceiptValidationIsEnabled(3);
	}

	// endregion
}}
