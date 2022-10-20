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

#include "catapult/consumers/BlockConsumers.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/BlockDifficultyCache.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/model/ChainScore.h"
#include "sdk/src/builders/NetworkConfigBuilder.h"
#include "tests/catapult/consumers/test/ConsumerInputFactory.h"
#include "tests/catapult/consumers/test/ConsumerTestUtils.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/EntityTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/mocks/MockMemoryBlockStorage.h"
#include "tests/test/nodeps/ParamsCapture.h"
#include "tests/TestHarness.h"
#include "plugins/txes/config/src/model/NetworkConfigTransaction.h"
#include "plugins/txes/aggregate/src/model/AggregateTransaction.h"

using catapult::disruptor::ConsumerInput;
using catapult::disruptor::InputSource;
using catapult::utils::HashSet;
using catapult::validators::ValidationResult;

namespace catapult { namespace consumers {

#define TEST_CLASS BlockChainSyncConsumerTests

	namespace {
		constexpr auto Base_Difficulty = Difficulty().unwrap();
		constexpr auto Two_In_60 = 1ll << 60;
		constexpr auto Max_Rollback_Blocks = 25u;
		constexpr model::ImportanceHeight Initial_Last_Recalculation_Height(1234);
		constexpr model::ImportanceHeight Modified_Last_Recalculation_Height(7777);
		const Key Sentinel_Processor_Public_Key = test::GenerateRandomByteArray<Key>();

		constexpr model::ImportanceHeight AddImportanceHeight(model::ImportanceHeight lhs, model::ImportanceHeight::ValueType rhs) {
			return model::ImportanceHeight(lhs.unwrap() + rhs);
		}

		// region RaisableErrorSource

		class RaisableErrorSource {
		public:
			RaisableErrorSource() : m_shouldRaiseError(false)
			{}

		public:
			void setError() {
				m_shouldRaiseError = true;
			}

		protected:
			void raise(const std::string& source) const {
				if (m_shouldRaiseError)
					CATAPULT_THROW_RUNTIME_ERROR(("raising error from " + source).c_str());
			}

		private:
			bool m_shouldRaiseError;
		};

		// endregion

		// region MockDifficultyChecker

		struct DifficultyCheckerParams {
		public:
			DifficultyCheckerParams(
				const std::vector<const model::Block*>& blocks,
				const cache::CatapultCache& cache,
				const model::NetworkConfigurations& remoteConfigs)
					: Blocks(blocks)
					, Cache(cache)
					, RemoteConfigs(remoteConfigs)
			{}

		public:
			const std::vector<const model::Block*> Blocks;
			const cache::CatapultCache& Cache;
			const model::NetworkConfigurations RemoteConfigs;
		};

		class MockDifficultyChecker : public test::ParamsCapture<DifficultyCheckerParams> {
		public:
			MockDifficultyChecker() : m_result(true)
			{}

		public:
			bool operator()(
					const std::vector<const model::Block*>& blocks,
					const cache::CatapultCache& cache,
					const model::NetworkConfigurations& remoteConfigs) const {
				const_cast<MockDifficultyChecker*>(this)->push(blocks, cache, remoteConfigs);
				return m_result;
			}

		public:
			void setFailure() {
				m_result = false;
			}

		private:
			bool m_result;
		};

		// endregion

		// region MockUndoBlock

		struct UndoBlockParams {
		public:
			UndoBlockParams(const model::BlockElement& blockElement, observers::ObserverState& state, UndoBlockType undoBlockType)
					: pBlock(test::CopyEntity(blockElement.Block))
					, UndoBlockType(undoBlockType)
					, LastRecalculationHeight(state.State.LastRecalculationHeight)
					, IsPassedMarkedCache(test::IsMarkedCache(state.Cache))
					, NumDifficultyInfos(state.Cache.sub<cache::BlockDifficultyCache>().size())
			{}

		public:
			std::shared_ptr<const model::Block> pBlock;
			consumers::UndoBlockType UndoBlockType;
			const model::ImportanceHeight LastRecalculationHeight;
			const bool IsPassedMarkedCache;
			const size_t NumDifficultyInfos;
		};

		class MockUndoBlock : public test::ParamsCapture<UndoBlockParams> {
		public:
			void operator()(const model::BlockElement& blockElement, observers::ObserverState& state, UndoBlockType undoBlockType) const {
				const_cast<MockUndoBlock*>(this)->push(blockElement, state, undoBlockType);

				// simulate undoing a block by modifying the state to mark it
				// if the block is common, it should not change any state (but it is still checked in assertUnwind)
				if (UndoBlockType::Common == undoBlockType)
					return;

				auto& blockDifficultyCache = state.Cache.sub<cache::BlockDifficultyCache>();
				blockDifficultyCache.insert(state::BlockDifficultyInfo(Height(blockDifficultyCache.size() + 1)));

				auto& height = state.State.LastRecalculationHeight;
				height = AddImportanceHeight(height, 1);
			}
		};

		// endregion

		// region MockProcessor

		struct ProcessorParams {
		public:
			ProcessorParams(const WeakBlockInfo& parentBlockInfo, const BlockElements& elements, observers::ObserverState& state)
					: pParentBlock(test::CopyEntity(parentBlockInfo.entity()))
					, ParentHash(parentBlockInfo.hash())
					, pElements(&elements)
					, LastRecalculationHeight(state.State.LastRecalculationHeight)
					, IsPassedMarkedCache(test::IsMarkedCache(state.Cache))
					, NumDifficultyInfos(state.Cache.sub<cache::BlockDifficultyCache>().size())
			{}

		public:
			std::shared_ptr<const model::Block> pParentBlock;
			const Hash256 ParentHash;
			const BlockElements* pElements;
			const model::ImportanceHeight LastRecalculationHeight;
			const bool IsPassedMarkedCache;
			const size_t NumDifficultyInfos;
		};

		class MockProcessor : public test::ParamsCapture<ProcessorParams> {
		public:
			MockProcessor() : m_result(ValidationResult::Success)
			{}

		public:
			ValidationResult operator()(
					const WeakBlockInfo& parentBlockInfo,
					BlockElements& elements,
					observers::ObserverState& state) const {
				const_cast<MockProcessor*>(this)->push(parentBlockInfo, elements, state);

				// mark the state by modifying it
				state.Cache.sub<cache::AccountStateCache>().addAccount(Sentinel_Processor_Public_Key, Height(1));
				state.State.LastRecalculationHeight = Modified_Last_Recalculation_Height;

				// modify all the elements
				for (auto& element : elements)
					element.GenerationHash = { { static_cast<uint8_t>(element.Block.Height.unwrap()) } };

				return m_result;
			}

		public:
			void setResult(ValidationResult result) {
				m_result = result;
			}

		private:
			ValidationResult m_result;
		};

		// endregion

		// region MockStateChange

		struct StateChangeParams {
		public:
			StateChangeParams(const subscribers::StateChangeInfo& changeInfo)
					: ScoreDelta(changeInfo.ScoreDelta)
					// all processing should have occurred before the state change notification,
					// so the sentinel account should have been added
					, IsPassedProcessedCache(HasMarkedChanges(changeInfo.CacheChanges))
					, Height(changeInfo.Height)
			{}

		public:
			model::ChainScore ScoreDelta;
			bool IsPassedProcessedCache;
			catapult::Height Height;

		private:
			static bool HasMarkedChanges(const cache::CacheChanges& changes) {
				auto addedAccountStates = changes.sub<cache::AccountStateCache>().addedElements();
				return std::any_of(addedAccountStates.cbegin(), addedAccountStates.cend(), [](const auto* pAccountState) {
					return Sentinel_Processor_Public_Key == pAccountState->PublicKey;
				});
			}
		};

		class MockStateChange : public test::ParamsCapture<StateChangeParams>, public RaisableErrorSource {
		public:
			void operator()(const subscribers::StateChangeInfo& changeInfo) const {
				raise("MockStateChange");
				const_cast<MockStateChange*>(this)->push(changeInfo);
			}
		};

		// endregion

		// region MockPreStateWritten

		struct PreStateWrittenParams {
		public:
			PreStateWrittenParams(const cache::CatapultCacheDelta& cacheDelta, const state::CatapultState& catapultState, Height height)
					// all processing should have occurred before the pre state written notification,
					// so the sentinel account should have been added
					: IsPassedProcessedCache(cacheDelta.sub<cache::AccountStateCache>().contains(Sentinel_Processor_Public_Key))
					, CatapultState(catapultState)
					, Height(height)
			{}

		public:
			bool IsPassedProcessedCache;
			state::CatapultState CatapultState;
			catapult::Height Height;
		};

		class MockPreStateWritten : public test::ParamsCapture<PreStateWrittenParams>, public RaisableErrorSource {
		public:
			void operator()(const cache::CatapultCacheDelta& cacheDelta, const state::CatapultState& catapultState, Height height) const {
				raise("MockPreStateWritten");
				const_cast<MockPreStateWritten*>(this)->push(cacheDelta, catapultState, height);
			}
		};

		// endregion

		// region MockTransactionsChange

		struct TransactionsChangeParams {
		public:
			TransactionsChangeParams(const HashSet& addedTransactionHashes, const HashSet& revertedTransactionHashes)
					: AddedTransactionHashes(addedTransactionHashes)
					, RevertedTransactionHashes(revertedTransactionHashes)
			{}

		public:
			const HashSet AddedTransactionHashes;
			const HashSet RevertedTransactionHashes;
		};

		class MockTransactionsChange : public test::ParamsCapture<TransactionsChangeParams> {
		public:
			void operator()(const TransactionsChangeInfo& changeInfo) const {
				TransactionsChangeParams params(
						CopyHashes(changeInfo.AddedTransactionHashes),
						CopyHashes(changeInfo.RevertedTransactionInfos));
				const_cast<MockTransactionsChange*>(this)->push(std::move(params));
			}

		private:
			static HashSet CopyHashes(const utils::HashPointerSet& hashPointers) {
				HashSet hashes;
				for (const auto* pHash : hashPointers)
					hashes.insert(*pHash);

				return hashes;
			}

			static HashSet CopyHashes(const std::vector<model::TransactionInfo>& transactionInfos) {
				HashSet hashes;
				for (const auto& transactionInfo : transactionInfos)
					hashes.insert(transactionInfo.EntityHash);

				return hashes;
			}
		};

		// endregion

		// region MockCommitStep

		class MockCommitStep : public test::ParamsCapture<CommitOperationStep> {
		public:
			void operator()(CommitOperationStep step) const {
				const_cast<MockCommitStep*>(this)->push(step);
			}
		};

		// endregion

		// region test utils

		void SetBlockHeight(model::Block& block, Height height) {
			block.Timestamp = Timestamp(height.unwrap() * 1000);
			block.Difficulty = Difficulty(16);
			block.Height = height;
		}

		std::vector<InputSource> GetAllInputSources() {
			return { InputSource::Unknown, InputSource::Local, InputSource::Remote_Pull, InputSource::Remote_Push };
		}

		void LogInputSource(InputSource source) {
			CATAPULT_LOG(debug) << "source " << source;
		}

		ConsumerInput CreateInput(Height startHeight, uint32_t numBlocks, InputSource source = InputSource::Remote_Pull) {
			auto input = test::CreateConsumerInputWithBlocks(numBlocks, source);
			auto nextHeight = startHeight;
			for (const auto& element : input.blocks()) {
				SetBlockHeight(const_cast<model::Block&>(element.Block), nextHeight);
				nextHeight = nextHeight + Height(1);
			}

			return input;
		}

		// endregion

		// region ConsumerTestContext

		struct ConsumerTestContext {
		public:
			ConsumerTestContext()
					: ConsumerTestContext(
							std::make_unique<mocks::MockMemoryBlockStorage>(),
							std::make_unique<mocks::MockMemoryBlockStorage>())
			{}

			explicit ConsumerTestContext(
					std::unique_ptr<io::BlockStorage>&& pStorage,
					std::unique_ptr<io::PrunableBlockStorage>&& pStagingStorage)
					: Cache(test::CreateCatapultCacheWithMarkerAccount())
					, Storage(std::move(pStorage), std::move(pStagingStorage)) {
				State.LastRecalculationHeight = Initial_Last_Recalculation_Height;

				BlockChainSyncHandlers handlers;
				handlers.DifficultyChecker = [this](const auto& blocks, const auto& cache, const auto& remoteConfigs) {
					return DifficultyChecker(blocks, cache, remoteConfigs);
				};
				handlers.UndoBlock = [this](const auto& block, auto& state, auto undoBlockType) {
					return UndoBlock(block, state, undoBlockType);
				};
				handlers.Processor = [this](const auto& parentBlockInfo, auto& elements, auto& state) {
					return Processor(parentBlockInfo, elements, state);
				};
				handlers.StateChange = [this](const auto& changeInfo) {
					return StateChange(changeInfo);
				};
				handlers.PreStateWritten = [this](const auto& cacheDelta, const auto& catapultState, auto height) {
					return PreStateWritten(cacheDelta, catapultState, height);
				};
				handlers.TransactionsChange = [this](const auto& changeInfo) {
					return TransactionsChange(changeInfo);
				};
				handlers.CommitStep = [this](auto step) {
					return CommitStep(step);
				};
				auto config = model::NetworkConfiguration::Uninitialized();
				config.MaxRollbackBlocks = Max_Rollback_Blocks;
				auto pConfigHolder = config::CreateMockConfigurationHolder(config);

				Consumer = CreateBlockChainSyncConsumer(Cache, State, Storage, pConfigHolder, handlers);
			}

		public:
			cache::CatapultCache Cache;
			state::CatapultState State;
			io::BlockStorageCache Storage;
			std::vector<std::shared_ptr<model::Block>> OriginalBlocks; // original stored blocks (excluding nemesis)

			MockDifficultyChecker DifficultyChecker;
			MockUndoBlock UndoBlock;
			MockProcessor Processor;
			MockStateChange StateChange;
			MockPreStateWritten PreStateWritten;
			MockTransactionsChange TransactionsChange;
			MockCommitStep CommitStep;

			disruptor::DisruptorConsumer Consumer;

		public:
			void seedStorage(Height desiredHeight, size_t numTransactionsPerBlock = 0, const test::ConstTransactions& desiredTransactions = {}) {
				// Arrange:
				auto height = Storage.view().chainHeight();
				auto storageModifier = Storage.modifier();

				while (height < desiredHeight) {
					height = height + Height(1);

					auto transactions = test::GenerateRandomTransactions(numTransactionsPerBlock);
					auto pBlock = desiredTransactions.empty() ?
						test::GenerateBlockWithTransactions(test::GenerateRandomTransactions(numTransactionsPerBlock)) :
						test::GenerateBlockWithTransactions(desiredTransactions);
					SetBlockHeight(*pBlock, height);

					// - seed with random tx hashes
					auto blockElement = test::BlockToBlockElement(*pBlock);
					for (auto& transactionElement : blockElement.Transactions)
						transactionElement.EntityHash = test::GenerateRandomByteArray<Hash256>();

					storageModifier.saveBlock(blockElement);
					OriginalBlocks.push_back(std::move(pBlock));
				}

				storageModifier.commit();
			}

		public:
			void assertDifficultyCheckerInvocation(const ConsumerInput& input) {
				// Assert:
				ASSERT_EQ(1u, DifficultyChecker.params().size());
				auto difficultyParams = DifficultyChecker.params()[0];

				EXPECT_EQ(&Cache, &difficultyParams.Cache);
				ASSERT_EQ(input.blocks().size(), difficultyParams.Blocks.size());
				for (auto i = 0u; i < input.blocks().size(); ++i)
					EXPECT_EQ(&input.blocks()[i].Block, difficultyParams.Blocks[i]) << "block at " << i;
			}

			void assertUnwind(const std::vector<Height>& unwoundHeights) {
				// Assert:
				ASSERT_EQ(unwoundHeights.size(), UndoBlock.params().size());
				auto i = 0u;
				for (auto height : unwoundHeights) {
					const auto& undoBlockParams = UndoBlock.params()[i];
					auto expectedHeight = AddImportanceHeight(Initial_Last_Recalculation_Height, i);
					auto expectedUndoType = i == unwoundHeights.size() - 1 ? UndoBlockType::Common : UndoBlockType::Rollback;
					auto message = "undo at " + std::to_string(i);

					EXPECT_EQ(*OriginalBlocks[(height - Height(2)).unwrap()], *undoBlockParams.pBlock) << message;
					EXPECT_EQ(expectedUndoType, undoBlockParams.UndoBlockType) << message;
					EXPECT_EQ(expectedHeight, undoBlockParams.LastRecalculationHeight) << message;
					EXPECT_TRUE(undoBlockParams.IsPassedMarkedCache) << message;
					EXPECT_EQ(i, undoBlockParams.NumDifficultyInfos) << message;
					++i;
				}
			}

			void assertProcessorInvocation(const ConsumerInput& input, size_t numUnwoundBlocks = 0) {
				// Assert:
				ASSERT_EQ(1u, Processor.params().size());
				const auto& processorParams = Processor.params()[0];
				auto expectedHeight = AddImportanceHeight(Initial_Last_Recalculation_Height, numUnwoundBlocks);
				auto pCommonBlockElement = Storage.view().loadBlockElement(input.blocks()[0].Block.Height - Height(1));

				EXPECT_EQ(pCommonBlockElement->Block, *processorParams.pParentBlock);
				EXPECT_EQ(pCommonBlockElement->EntityHash, processorParams.ParentHash);
				EXPECT_EQ(&input.blocks(), processorParams.pElements);
				EXPECT_EQ(expectedHeight, processorParams.LastRecalculationHeight);
				EXPECT_TRUE(processorParams.IsPassedMarkedCache);
				EXPECT_EQ(numUnwoundBlocks, processorParams.NumDifficultyInfos);
			}

			void assertNoStorageChanges() {
				// Assert: all original blocks are present in the storage
				auto storageView = Storage.view();
				ASSERT_EQ(Height(OriginalBlocks.size()) + Height(1), storageView.chainHeight());
				for (const auto& pBlock : OriginalBlocks) {
					auto pStorageBlock = storageView.loadBlock(pBlock->Height);
					EXPECT_EQ(*pBlock, *pStorageBlock) << "at height " << pBlock->Height;
				}

				// - the cache was not committed
				EXPECT_FALSE(Cache.sub<cache::AccountStateCache>().createView(Height{0})->contains(Sentinel_Processor_Public_Key));
				EXPECT_EQ(0u, Cache.sub<cache::BlockDifficultyCache>().createView(Height{0})->size());

				// - no state changes were announced
				EXPECT_EQ(0u, StateChange.params().size());
				EXPECT_EQ(0u, PreStateWritten.params().size());

				// - the state was not changed
				EXPECT_EQ(Initial_Last_Recalculation_Height, State.LastRecalculationHeight);

				// - no transaction changes were announced
				EXPECT_EQ(0u, TransactionsChange.params().size());

				// - no commit steps were announced
				EXPECT_EQ(0u, CommitStep.params().size());
			}

			void assertStored(const ConsumerInput& input, const model::ChainScore& expectedScoreDelta) {
				// Assert: all input blocks should be saved in the storage
				auto storageView = Storage.view();
				auto inputHeight = input.blocks()[0].Block.Height;
				auto chainHeight = storageView.chainHeight();
				ASSERT_EQ(inputHeight + Height(input.blocks().size() - 1), chainHeight);
				for (auto height = inputHeight; height <= chainHeight; height = height + Height(1)) {
					auto pStorageBlock = storageView.loadBlock(height);
					EXPECT_EQ(input.blocks()[(height - inputHeight).unwrap()].Block, *pStorageBlock) << "at height " << height;
				}

				// - non conflicting original blocks should still be in storage
				for (auto height = Height(2); height < inputHeight; height = height + Height(1)) {
					auto pStorageBlock = storageView.loadBlock(height);
					EXPECT_EQ(*OriginalBlocks[(height - Height(2)).unwrap()], *pStorageBlock) << "at height " << height;
				}

				// - the cache was committed (add 1 to OriginalBlocks.size() because it does not include the nemesis)
				EXPECT_TRUE(Cache.sub<cache::AccountStateCache>().createView(Height{0})->contains(Sentinel_Processor_Public_Key));
				EXPECT_EQ(
						OriginalBlocks.size() + 1 - inputHeight.unwrap() + 1,
						Cache.sub<cache::BlockDifficultyCache>().createView(Height{0})->size());
				EXPECT_EQ(chainHeight, Cache.createView().height());

				// - state changes were announced
				ASSERT_EQ(1u, StateChange.params().size());
				const auto& stateChangeParams = StateChange.params()[0];
				EXPECT_EQ(expectedScoreDelta, stateChangeParams.ScoreDelta);
				EXPECT_TRUE(stateChangeParams.IsPassedProcessedCache);
				EXPECT_EQ(chainHeight, stateChangeParams.Height);

				// - pre state written checkpoint was announced
				ASSERT_EQ(1u, PreStateWritten.params().size());
				const auto& preStateWrittenParams = PreStateWritten.params()[0];
				EXPECT_TRUE(preStateWrittenParams.IsPassedProcessedCache);
				EXPECT_EQ(Modified_Last_Recalculation_Height, preStateWrittenParams.CatapultState.LastRecalculationHeight);
				EXPECT_EQ(chainHeight, preStateWrittenParams.Height);

				// - the state was actually changed
				EXPECT_EQ(Modified_Last_Recalculation_Height, State.LastRecalculationHeight);

				// - transaction changes were announced
				EXPECT_EQ(1u, TransactionsChange.params().size());

				// - commit steps were announced
				ASSERT_EQ(3u, CommitStep.params().size());
				EXPECT_EQ(CommitOperationStep::Blocks_Written, CommitStep.params()[0]);
				EXPECT_EQ(CommitOperationStep::State_Written, CommitStep.params()[1]);
				EXPECT_EQ(CommitOperationStep::All_Updated, CommitStep.params()[2]);
			}
		};

		// endregion
	}

	// region basic

	TEST(TEST_CLASS, CanProcessZeroEntities) {
		// Arrange:
		ConsumerTestContext context;

		// Assert:
		test::AssertPassthroughForEmptyInput(context.Consumer);
	}

	// endregion

	// region height check

	namespace {
		void AssertInvalidHeightWithResult(
				Height localHeight,
				Height remoteHeight,
				uint32_t numRemoteBlocks,
				InputSource source,
				ValidationResult expectedResult) {
			// Arrange:
			ConsumerTestContext context;
			context.seedStorage(localHeight);
			auto input = CreateInput(remoteHeight, numRemoteBlocks, source);

			// Act:
			auto result = context.Consumer(input);

			// Assert:
			test::AssertAborted(result, expectedResult);
			EXPECT_EQ(0u, context.DifficultyChecker.params().size());
			EXPECT_EQ(0u, context.UndoBlock.params().size());
			EXPECT_EQ(0u, context.Processor.params().size());
			context.assertNoStorageChanges();
		}

		void AssertInvalidHeight(Height localHeight, Height remoteHeight, uint32_t numRemoteBlocks, InputSource source) {
			// Assert:
			AssertInvalidHeightWithResult(localHeight, remoteHeight, numRemoteBlocks, source, Failure_Consumer_Remote_Chain_Unlinked);
		}

		void AssertValidHeight(Height localHeight, Height remoteHeight, uint32_t numRemoteBlocks, InputSource source) {
			// Arrange:
			ConsumerTestContext context;
			context.seedStorage(localHeight);
			auto input = CreateInput(remoteHeight, numRemoteBlocks, source);

			// Act:
			context.Consumer(input);

			// Assert: if the height is valid, the difficulty checker must have been called
			EXPECT_EQ(1u, context.DifficultyChecker.params().size());
		}
	}

	TEST(TEST_CLASS, RemoteChainWithHeightLessThanTwoIsRejected) {
		// Assert:
		for (auto source : GetAllInputSources()) {
			LogInputSource(source);
			AssertInvalidHeight(Height(1), Height(0), 3, source);
			AssertInvalidHeight(Height(1), Height(1), 3, source);
		}
	}

	TEST(TEST_CLASS, RemoteChainWithHeightAtLeastTwoIsValid) {
		// Assert:
		for (auto source : GetAllInputSources()) {
			LogInputSource(source);
			AssertValidHeight(Height(1), Height(2), 3, source);
			AssertValidHeight(Height(2), Height(3), 3, source);
		}
	}

	TEST(TEST_CLASS, RemoteChainWithHeightMoreThanOneGreaterThanLocalHeightIsRejected) {
		// Assert:
		for (auto source : GetAllInputSources()) {
			LogInputSource(source);
			AssertInvalidHeight(Height(100), Height(102), 3, source);
			AssertInvalidHeight(Height(100), Height(200), 3, source);
		}
	}

	TEST(TEST_CLASS, RemoteChainWithHeightLessThanLocalHeightIsOnlyValidForRemotePullSource) {
		// Assert:
		for (auto source : GetAllInputSources()) {
			LogInputSource(source);
			auto assertFunc = InputSource::Remote_Pull == source ? AssertValidHeight : AssertInvalidHeight;
			assertFunc(Height(100), Height(99), 1, source);
			assertFunc(Height(100), Height(90), 1, source);
		}
	}

	TEST(TEST_CLASS, RemoteChainWithHeightAtOrOneGreaterThanLocalHeightIsValidForAllSources) {
		// Assert:
		for (auto source : GetAllInputSources()) {
			LogInputSource(source);
			AssertValidHeight(Height(100), Height(100), 1, source);
			AssertValidHeight(Height(100), Height(101), 1, source);
		}
	}

	TEST(TEST_CLASS, RemoteChainWithHeightDifferenceEqualToMaxRollbackBlocksIsValidForRemotePullSource) {
		// Assert: (this test only makes sense for Remote_Pull because it is the only source that allows multiple block rollbacks)
		AssertValidHeight(Height(100), Height(100 - Max_Rollback_Blocks), 4, InputSource::Remote_Pull);
	}

	TEST(TEST_CLASS, RemoteChainWithHeightDifferencGreaterThanMaxRollbackBlocksIsInvalidForRemotePullSource) {
		// Assert: (this test only makes sense for Remote_Pull because it is the only source that allows multiple block rollbacks)
		auto expectedResult = Failure_Consumer_Remote_Chain_Too_Far_Behind;
		AssertInvalidHeightWithResult(Height(100), Height(100 - Max_Rollback_Blocks - 1), 4, InputSource::Remote_Pull, expectedResult);
		AssertInvalidHeightWithResult(Height(100), Height(100 - Max_Rollback_Blocks - 10), 4, InputSource::Remote_Pull, expectedResult);
	}

	// endregion

	// region difficulties check

	TEST(TEST_CLASS, RemoteChainWithIncorrectDifficultiesIsRejected) {
		// Arrange: trigger a difficulty check failure
		ConsumerTestContext context;
		context.seedStorage(Height(3));
		context.DifficultyChecker.setFailure();

		auto input = CreateInput(Height(4), 2);

		// Act:
		auto result = context.Consumer(input);

		// Assert:
		test::AssertAborted(result, Failure_Consumer_Remote_Chain_Mismatched_Difficulties);
		EXPECT_EQ(0u, context.UndoBlock.params().size());
		EXPECT_EQ(0u, context.Processor.params().size());
		context.assertDifficultyCheckerInvocation(input);
		context.assertNoStorageChanges();
	}

	// endregion

	// region chain score test

	TEST(TEST_CLASS, ChainWithSmallerScoreIsRejected) {
		// Arrange: create a local storage with blocks 1-7 and a remote storage with blocks 5-6
		//          (note that the test setup ensures scores are linearly correlated with number of blocks)
		ConsumerTestContext context;
		context.seedStorage(Height(7));
		auto input = CreateInput(Height(5), 2);

		// Act:
		auto result = context.Consumer(input);

		// Assert:
		test::AssertAborted(result, Failure_Consumer_Remote_Chain_Score_Not_Better);
		EXPECT_EQ(4u, context.UndoBlock.params().size());
		EXPECT_EQ(0u, context.Processor.params().size());
		context.assertDifficultyCheckerInvocation(input);
		context.assertUnwind({ Height(7), Height(6), Height(5), Height(4) });
		context.assertNoStorageChanges();
	}

	TEST(TEST_CLASS, ChainWithIdenticalScoreIsRejected) {
		// Arrange: create a local storage with blocks 1-7 and a remote storage with blocks 6-7
		//          (note that the test setup ensures scores are linearly correlated with number of blocks)
		ConsumerTestContext context;
		context.seedStorage(Height(7));
		auto input = CreateInput(Height(6), 2);

		// Act:
		auto result = context.Consumer(input);

		// Assert:
		test::AssertAborted(result, Failure_Consumer_Remote_Chain_Score_Not_Better);
		EXPECT_EQ(3u, context.UndoBlock.params().size());
		EXPECT_EQ(0u, context.Processor.params().size());
		context.assertDifficultyCheckerInvocation(input);
		context.assertUnwind({ Height(7), Height(6), Height(5) });
		context.assertNoStorageChanges();
	}

	// endregion

	// region processor check

	namespace {
		void AssertRemoteChainWithNonSuccessProcessorResultIsRejected(ValidationResult processorResult) {
			// Arrange: configure the processor to return a non-success result
			ConsumerTestContext context;
			context.seedStorage(Height(3));
			context.Processor.setResult(processorResult);

			auto input = CreateInput(Height(4), 2);

			// Act:
			auto result = context.Consumer(input);

			// Assert:
			test::AssertAborted(result, processorResult);
			EXPECT_EQ(0u, context.UndoBlock.params().size());
			context.assertDifficultyCheckerInvocation(input);
			context.assertProcessorInvocation(input);
			context.assertNoStorageChanges();
		}
	}

	TEST(TEST_CLASS, RemoteChainWithProcessorFailureIsRejected_Neutral) {
		// Assert:
		AssertRemoteChainWithNonSuccessProcessorResultIsRejected(ValidationResult::Neutral);
	}

	TEST(TEST_CLASS, RemoteChainWithProcessorFailureIsRejected_Failure) {
		// Assert:
		AssertRemoteChainWithNonSuccessProcessorResultIsRejected(ValidationResult::Failure);
	}

	// endregion

	// region successful syncs

	TEST(TEST_CLASS, CanSyncCompatibleChains) {
		// Arrange: create a local storage with blocks 1-7 and a remote storage with blocks 8-11
		ConsumerTestContext context;
		context.seedStorage(Height(7));
		auto input = CreateInput(Height(8), 4);

		// Act:
		auto result = context.Consumer(input);

		// Assert:
		test::AssertContinued(result);
		EXPECT_EQ(0u, context.UndoBlock.params().size());
		context.assertDifficultyCheckerInvocation(input);
		context.assertProcessorInvocation(input);
		context.assertStored(input, model::ChainScore(4 * Two_In_60));
	}

	TEST(TEST_CLASS, CanSyncIncompatibleChains) {
		// Arrange: create a local storage with blocks 1-7 and a remote storage with blocks 5-8
		ConsumerTestContext context;
		context.seedStorage(Height(7));
		auto input = CreateInput(Height(5), 4);

		// Act:
		auto result = context.Consumer(input);

		// Assert:
		test::AssertContinued(result);
		EXPECT_EQ(4u, context.UndoBlock.params().size());
		context.assertDifficultyCheckerInvocation(input);
		context.assertUnwind({ Height(7), Height(6), Height(5), Height(4) });
		context.assertProcessorInvocation(input, 3);
		context.assertStored(input, model::ChainScore(Two_In_60));
	}

	TEST(TEST_CLASS, CanSyncIncompatibleChainsWithOnlyLastBlockDifferent) {
		// Arrange: create a local storage with blocks 1-7 and a remote storage with blocks 7-10
		ConsumerTestContext context;
		context.seedStorage(Height(7));
		auto input = CreateInput(Height(7), 4);

		// Act:
		auto result = context.Consumer(input);

		// Assert:
		test::AssertContinued(result);
		EXPECT_EQ(2u, context.UndoBlock.params().size());
		context.assertDifficultyCheckerInvocation(input);
		context.assertUnwind({ Height(7), Height(6) });
		context.assertProcessorInvocation(input, 1);
		context.assertStored(input, model::ChainScore(3 * Two_In_60));
	}

	TEST(TEST_CLASS, CanSyncIncompatibleChainsWhereShorterRemoteChainHasHigherScore) {
		// Arrange: create a local storage with blocks 1-7 and a remote storage with blocks 5
		ConsumerTestContext context;
		context.seedStorage(Height(7));
		auto input = CreateInput(Height(5), 1);
		for (auto& blockElement : input.blocks()) {
			const_cast<model::Block&>(blockElement.Block).Difficulty = Difficulty(4);
		}

		// Act:
		auto result = context.Consumer(input);

		// Assert:
		test::AssertContinued(result);
		EXPECT_EQ(4u, context.UndoBlock.params().size());
		context.assertDifficultyCheckerInvocation(input);
		context.assertUnwind({ Height(7), Height(6), Height(5), Height(4) });
		context.assertProcessorInvocation(input, 3);
		uint64_t Two_In_62 = 1ll << 62;
		// We undo 3 blocks with diff 2^60, and add a new block with diff 2^62
		context.assertStored(input, model::ChainScore(Two_In_62 - 3ll * Two_In_60));
	}

	// endregion

	// region transaction notification

	namespace {
		template<typename TContainer, typename TKey>
		bool Contains(const TContainer& container, const TKey& key) {
			return container.cend() != container.find(key);
		}

		void AssertHashesAreEqual(const std::vector<Hash256>& expected, const HashSet& actual) {
			EXPECT_EQ(expected.size(), actual.size());

			auto i = 0u;
			for (const auto& hash : expected)
				EXPECT_TRUE(Contains(actual, hash)) << "hash at " << i++;
		}

		class InputTransactionBuilder {
		public:
			explicit InputTransactionBuilder(ConsumerInput& input) : m_input(input)
			{}

		public:
			const std::vector<Hash256>& hashes() const {
				return m_addedHashes;
			}

		public:
			void addRandom(size_t elementIndex, size_t numTransactions) {
				for (auto i = 0u; i < numTransactions; ++i)
					add(elementIndex, test::GenerateRandomTransaction(), test::GenerateRandomByteArray<Hash256>());
			}

			void addFromStorage(size_t elementIndex, const io::BlockStorageCache& storage, Height height, size_t txIndex) {
				auto pBlockElement = storage.view().loadBlockElement(height);

				auto i = 0u;
				for (const auto& transactionElement : pBlockElement->Transactions) {
					if (i++ != txIndex)
						continue;

					add(elementIndex, test::CopyEntity(transactionElement.Transaction), transactionElement.EntityHash);
					break;
				}
			}

		private:
			void add(size_t elementIndex, const std::shared_ptr<model::Transaction>& pTransaction, const Hash256& hash) {
				auto transactionElement = model::TransactionElement(*pTransaction);
				transactionElement.EntityHash = hash;

				m_input.blocks()[elementIndex].Transactions.push_back(transactionElement);
				m_addedHashes.push_back(hash);
				m_transactions.push_back(pTransaction); // keep the transaction alive
			}

		private:
			ConsumerInput& m_input;
			std::vector<Hash256> m_addedHashes;
			std::vector<std::shared_ptr<model::Transaction>> m_transactions;
		};

		std::vector<Hash256> ExtractTransactionHashesFromStorage(
				const io::BlockStorageView& storage,
				Height startHeight,
				Height endHeight) {
			std::vector<Hash256> hashes;
			for (auto h = startHeight; h <= endHeight; h = h + Height(1)) {
				auto pBlockElement = storage.loadBlockElement(h);
				for (const auto& transactionElement : pBlockElement->Transactions)
					hashes.push_back(transactionElement.EntityHash);
			}

			return hashes;
		}
	}

	TEST(TEST_CLASS, CanSyncCompatibleChains_TransactionNotification) {
		// Arrange: create a local storage with blocks 1-7 and a remote storage with blocks 8-11
		ConsumerTestContext context;
		context.seedStorage(Height(7), 3);
		auto input = CreateInput(Height(8), 4);

		// - add transactions to the input
		InputTransactionBuilder builder(input);
		builder.addRandom(0, 1);
		builder.addRandom(2, 3);
		builder.addRandom(3, 2);

		// Act:
		auto result = context.Consumer(input);

		// Assert:
		test::AssertContinued(result);
		EXPECT_EQ(0u, context.UndoBlock.params().size());
		context.assertDifficultyCheckerInvocation(input);
		context.assertProcessorInvocation(input);
		context.assertStored(input, model::ChainScore(4 * Two_In_60));

		// - the change notification had 6 added and 0 reverted
		ASSERT_EQ(1u, context.TransactionsChange.params().size());
		const auto& txChangeParams = context.TransactionsChange.params()[0];

		EXPECT_EQ(6u, txChangeParams.AddedTransactionHashes.size());
		AssertHashesAreEqual(builder.hashes(), txChangeParams.AddedTransactionHashes);

		EXPECT_TRUE(txChangeParams.RevertedTransactionHashes.empty());
	}

	TEST(TEST_CLASS, CanSyncIncompatibleChains_TransactionNotification) {
		// Arrange: create a local storage with blocks 1-7 and a remote storage with blocks 5-8
		ConsumerTestContext context;
		context.seedStorage(Height(7), 3);
		auto input = CreateInput(Height(5), 4);

		// - add transactions to the input
		InputTransactionBuilder builder(input);
		builder.addRandom(0, 1);
		builder.addRandom(2, 3);
		builder.addRandom(3, 2);

		// - extract original hashes from storage
		auto expectedRevertedHashes = ExtractTransactionHashesFromStorage(context.Storage.view(), Height(5), Height(7));

		// Act:
		auto result = context.Consumer(input);

		// Assert:
		test::AssertContinued(result);
		EXPECT_EQ(4u, context.UndoBlock.params().size());
		context.assertDifficultyCheckerInvocation(input);
		context.assertUnwind({ Height(7), Height(6), Height(5), Height(4) });
		context.assertProcessorInvocation(input, 3);
		context.assertStored(input, model::ChainScore(Two_In_60));

		// - the change notification had 6 added and 9 reverted
		ASSERT_EQ(1u, context.TransactionsChange.params().size());
		const auto& txChangeParams = context.TransactionsChange.params()[0];

		EXPECT_EQ(6u, txChangeParams.AddedTransactionHashes.size());
		AssertHashesAreEqual(builder.hashes(), txChangeParams.AddedTransactionHashes);

		EXPECT_EQ(9u, txChangeParams.RevertedTransactionHashes.size());
		AssertHashesAreEqual(expectedRevertedHashes, txChangeParams.RevertedTransactionHashes);
	}

	TEST(TEST_CLASS, CanSyncIncompatibleChainsWithSharedTransacions_TransactionNotification) {
		// Arrange: create a local storage with blocks 1-7 and a remote storage with blocks 5-8
		ConsumerTestContext context;
		context.seedStorage(Height(7), 3);
		auto input = CreateInput(Height(5), 4);

		// - add transactions to the input
		InputTransactionBuilder builder(input);
		builder.addRandom(0, 1);
		builder.addRandom(2, 3);
		builder.addRandom(3, 2);
		builder.addFromStorage(2, context.Storage, Height(5), 2);
		builder.addFromStorage(0, context.Storage, Height(7), 1);

		// - extract original hashes from storage
		auto expectedRevertedHashes = ExtractTransactionHashesFromStorage(context.Storage.view(), Height(5), Height(7));
		expectedRevertedHashes.erase(expectedRevertedHashes.begin() + 2 * 3 + 1); // block 7 tx 2
		expectedRevertedHashes.erase(expectedRevertedHashes.begin() + 2); // block 5 tx 3

		// Act:
		auto result = context.Consumer(input);

		// Assert:
		test::AssertContinued(result);
		EXPECT_EQ(4u, context.UndoBlock.params().size());
		context.assertDifficultyCheckerInvocation(input);
		context.assertUnwind({ Height(7), Height(6), Height(5), Height(4) });
		context.assertProcessorInvocation(input, 3);
		context.assertStored(input, model::ChainScore(Two_In_60));

		// - the change notification had 8 added and 7 reverted
		ASSERT_EQ(1u, context.TransactionsChange.params().size());
		const auto& txChangeParams = context.TransactionsChange.params()[0];

		EXPECT_EQ(8u, txChangeParams.AddedTransactionHashes.size());
		AssertHashesAreEqual(builder.hashes(), txChangeParams.AddedTransactionHashes);

		EXPECT_EQ(7u, txChangeParams.RevertedTransactionHashes.size());
		AssertHashesAreEqual(expectedRevertedHashes, txChangeParams.RevertedTransactionHashes);
	}

	// endregion

	// region element updates

	TEST(TEST_CLASS, AllowsUpdateOfInputElements) {
		// Arrange: create a local storage with blocks 1-7 and a remote storage with blocks 8-11
		ConsumerTestContext context;
		context.seedStorage(Height(7));
		auto input = CreateInput(Height(8), 4);

		// Sanity: clear all generation hashes
		for (auto& blockElement : input.blocks())
			blockElement.GenerationHash = {};

		// Act:
		auto result = context.Consumer(input);

		// Sanity:
		test::AssertContinued(result);

		// Assert: the input generation hashes were updated
		uint8_t i = 8;
		for (const auto& blockElement : input.blocks()) {
			GenerationHash expectedGenerationHash{ { i++ } };
			EXPECT_EQ(expectedGenerationHash, blockElement.GenerationHash) << "generation hash at " << i;
		}
	}

	// endregion

	// region step notification

	namespace {
		class ErrorAwareBlockStorage : public mocks::MockMemoryBlockStorage, public RaisableErrorSource {
		public:
			void dropBlocksAfter(Height height) override {
				MockMemoryBlockStorage::dropBlocksAfter(height);
				raise("ErrorAwareBlockStorage::dropBlocksAfter");
			}
		};
	}

	TEST(TEST_CLASS, CommitStepsAreCorrectWhenWritingBlocksFails) {
		// Arrange:
		auto pBlockStorage = std::make_unique<ErrorAwareBlockStorage>();
		auto* pBlockStorageRaw = pBlockStorage.get();

		ConsumerTestContext context(std::make_unique<ErrorAwareBlockStorage>(), std::move(pBlockStorage));
		context.seedStorage(Height(7));
		auto input = CreateInput(Height(6), 4);

		// - simulate block writing failure
		pBlockStorageRaw->setError();

		// Act:
		EXPECT_THROW(context.Consumer(input), catapult_runtime_error);

		// Assert:
		EXPECT_EQ(0u, context.CommitStep.params().size());
	}

	TEST(TEST_CLASS, CommitStepsAreCorrectWhenWritingStateFails) {
		// Arrange:
		ConsumerTestContext context;
		context.seedStorage(Height(7));
		auto input = CreateInput(Height(6), 4);

		// - simulate state writing failure
		context.PreStateWritten.setError();

		// Act:
		EXPECT_THROW(context.Consumer(input), catapult_runtime_error);

		// Assert:
		ASSERT_EQ(1u, context.CommitStep.params().size());
		EXPECT_EQ(CommitOperationStep::Blocks_Written, context.CommitStep.params()[0]);
	}

	TEST(TEST_CLASS, CommitStepsAreCorrectWhenUpdatingFails) {
		// Arrange:
		auto pBlockStorage = std::make_unique<ErrorAwareBlockStorage>();
		auto* pBlockStorageRaw = pBlockStorage.get();

		ConsumerTestContext context(std::move(pBlockStorage), std::make_unique<ErrorAwareBlockStorage>());
		context.seedStorage(Height(7));
		auto input = CreateInput(Height(6), 4);

		// - simulate block updating failure
		pBlockStorageRaw->setError();

		// Act:
		EXPECT_THROW(context.Consumer(input), catapult_runtime_error);

		// Assert:
		ASSERT_EQ(2u, context.CommitStep.params().size());
		EXPECT_EQ(CommitOperationStep::Blocks_Written, context.CommitStep.params()[0]);
		EXPECT_EQ(CommitOperationStep::State_Written, context.CommitStep.params()[1]);
	}

	// endregion

	// region network config transaction

	namespace {
		template<typename TTraits>
		void AssertBlocksWithNetworkConfigTransaction(const BlockDuration& applyHeightDelta) {
			// Arrange: create a local storage with blocks 1-20.
			// And a remote storage with blocks 8-16 with config transaction at height 12.
			Height localHeight(20);
			Height configTransactionHeight(12);
			uint64_t numRemoteBlocks = 9;
			auto numBlocksBefore = numRemoteBlocks / 2;
			auto numBlocksAfter = numBlocksBefore;

			// Seed local storage.
			ConsumerTestContext context;
			test::ConstTransactions networkConfigTransaction{ TTraits::CreateConfigTransaction(applyHeightDelta) };
			context.seedStorage(localHeight);

			// Create remote block input.
			auto input = CreateInput(configTransactionHeight - Height(numBlocksBefore), numBlocksBefore);
			auto pBlock = test::GenerateBlockWithTransactions(test::TestBlockTransactions(networkConfigTransaction));
			SetBlockHeight(*pBlock, configTransactionHeight);
			input.blocks().push_back(test::BlockToBlockElement(*pBlock));
			auto input2 = CreateInput(configTransactionHeight + Height(1), numBlocksAfter);
			for (const auto& element : input2.blocks())
				input.blocks().push_back(element);
			for (auto& element : input.blocks())
				const_cast<model::Block&>(element.Block).Difficulty = Difficulty(5);

			// Act:
			auto result = context.Consumer(input);

			// Assert:
			test::AssertContinued(result);
			auto numInputBlocks = std::min(numBlocksBefore + applyHeightDelta.unwrap(), numRemoteBlocks);
			EXPECT_EQ(numInputBlocks, input.blocks().size());
			context.assertProcessorInvocation(input, (localHeight - configTransactionHeight).unwrap() + numBlocksBefore + 1);
			ASSERT_EQ(1u, context.DifficultyChecker.params().size());
			EXPECT_EQ(1u, context.DifficultyChecker.params()[0].RemoteConfigs.size());
		}
	}
	namespace {
		auto CreateConfigTransactionBuilder(const BlockDuration& applyHeightDelta) {
			builders::NetworkConfigBuilder<model::NetworkConfigTransaction> builder(model::NetworkIdentifier::Zero, Key());
			builder.setApplyHeightDelta(BlockDuration{applyHeightDelta});
			auto resourcesPath = boost::filesystem::path("../resources");
			builder.setBlockChainConfig((resourcesPath / "config-network.properties").generic_string());

			return builder;
		}

		struct RegularTraits {
			static auto CreateConfigTransaction(const BlockDuration& applyHeightDelta) {
				return CreateConfigTransactionBuilder(applyHeightDelta).build();
			}
		};

		template<typename TDescriptor>
		struct EmbeddedTraits {
			static auto CreateConfigTransaction(const BlockDuration& applyHeightDelta) {
				auto pEmbeddedTransaction = CreateConfigTransactionBuilder(applyHeightDelta).buildEmbedded();

				uint32_t size = sizeof(model::AggregateTransaction<TDescriptor>) + pEmbeddedTransaction->Size;
				auto pTransaction = utils::MakeUniqueWithSize<model::AggregateTransaction<TDescriptor>>(size);
				if constexpr(std::is_same_v<TDescriptor, model::AggregateTransactionRawDescriptor>)
					pTransaction->Version = model::MakeVersion(model::NetworkIdentifier::Mijin_Test, 2);
				else
					pTransaction->Version = model::MakeVersion(model::NetworkIdentifier::Mijin_Test, 1);

				pTransaction->Type = model::AggregateTransaction<TDescriptor>::Entity_Type;
				pTransaction->Size = size;
				pTransaction->PayloadSize = size - sizeof(model::AggregateTransaction<TDescriptor>);

				std::memcpy(pTransaction.get() + 1, pEmbeddedTransaction.get(), pEmbeddedTransaction->Size);

				return pTransaction;
			}
		};
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_RegularConfigTransaction) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<RegularTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_EmbeddedConfigTransaction_V1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<EmbeddedTraits<model::AggregateTransactionRawDescriptor>>(); } \
	TEST(TEST_CLASS, TEST_NAME##_EmbeddedConfigTransaction_V2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<EmbeddedTraits<model::AggregateTransactionExtendedDescriptor>>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TRAITS_BASED_TEST(CommitBlocksBeforeNetworkConfig) {
		AssertBlocksWithNetworkConfigTransaction<TTraits>(BlockDuration(2));
	}

	TRAITS_BASED_TEST(CommitAllBlocksWhenNetworkConfigNotAppliedInNewBlocks) {
		AssertBlocksWithNetworkConfigTransaction<TTraits>(BlockDuration(10));
	}

	// endregion
}}
