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

#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/consumers/BlockConsumers.h"
#include "catapult/consumers/BlockChainSyncConsumer.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "tests/catapult/consumers/test/ConsumerInputFactory.h"
#include "tests/catapult/consumers/test/ConsumerTestUtils.h"
#include "tests/catapult/extensions/test/LocalNodeStateUtils.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/nodeps/ParamsCapture.h"
#include "tests/TestHarness.h"

using catapult::disruptor::ConsumerInput;
using catapult::disruptor::InputSource;
using catapult::utils::HashSet;
using catapult::validators::ValidationResult;

namespace catapult { namespace consumers {

#define TEST_CLASS BlockChainSyncConsumerTests

	namespace {
		constexpr auto Base_Difficulty = Difficulty().unwrap();
		constexpr auto Max_Rollback_Blocks = 25u;
		constexpr auto Effective_Balance = 1440u;
		const Key Sentinel_Processor_Public_Key = test::GenerateRandomData<Key_Size>();

		// region MockDifficultyChecker

		struct DifficultyCheckerParams {
		public:
			DifficultyCheckerParams(const model::Block& remoteBlock, const model::Block& localBlock)
					: RemoteBlock(test::CopyBlock(remoteBlock))
					, LocalBlock(test::CopyBlock(localBlock))
			{}

		public:
			std::shared_ptr<model::Block> RemoteBlock;
			std::shared_ptr<model::Block> LocalBlock;
		};

		class MockDifficultyChecker : public test::ParamsCapture<DifficultyCheckerParams> {
		public:
			MockDifficultyChecker() : m_result(true)
			{}

		public:
			bool operator()(const model::Block& remoteBlock, const model::Block& localBlock) const {
				const_cast<MockDifficultyChecker*>(this)->push(remoteBlock, localBlock);
				return m_result;
			}

		public:
			void setResult(bool result) {
				m_result = result;
			}

		private:
			bool m_result;
		};

		// endregion

		// region MockUndoBlock

		struct UndoBlockParams {
		public:
			UndoBlockParams(const model::BlockElement& blockElement, const observers::ObserverState& state)
					: pBlock(test::CopyBlock(blockElement.Block))
					, IsPassedMarkedCache(test::IsMarkedCache(state.Cache))
			{}

		public:
			std::shared_ptr<const model::Block> pBlock;
			const bool IsPassedMarkedCache;
		};

		class MockUndoBlock : public test::ParamsCapture<UndoBlockParams> {
		public:
			void operator()(const model::BlockElement& blockElement, const observers::ObserverState& state) const {
				const_cast<MockUndoBlock*>(this)->push(blockElement, state);
			}
		};

		// endregion

		// region MockProcessor

		struct ProcessorParams {
		public:
			ProcessorParams(SyncState& state, BlockElements& elements)
					: pParentBlock(test::CopyBlock(state.commonBlockInfo().entity()))
					, ParentHash(state.commonBlockInfo().hash())
					, pElements(&elements)
					, IsPassedMarkedCache(test::IsMarkedCache(state.currentObserverState().Cache))
			{}

		public:
			std::shared_ptr<const model::Block> pParentBlock;
			const Hash256 ParentHash;
			const BlockElements* pElements;
			const bool IsPassedMarkedCache;
		};

		class MockProcessor : public test::ParamsCapture<ProcessorParams> {
		public:
			MockProcessor() : m_result(ValidationResult::Success)
			{}

		public:
			ValidationResult operator()(SyncState& state, BlockElements& elements) const {
				const_cast<MockProcessor*>(this)->push(state, elements);

				// mark the state by modifying it
				auto observerState = state.currentObserverState();
				observerState.Cache.sub<cache::AccountStateCache>().addAccount(Sentinel_Processor_Public_Key, Height(1));

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
			StateChangeParams(const StateChangeInfo& changeInfo)
					// all processing should have occurred before the state change notification,
					// so the sentinel account should have been added
					: IsPassedMarkedCache(changeInfo.CacheDelta.sub<cache::AccountStateCache>().contains(Sentinel_Processor_Public_Key))
					, Height(changeInfo.Height)
			{}

		public:
			bool IsPassedMarkedCache;
			catapult::Height Height;
		};

		class MockStateChange : public test::ParamsCapture<StateChangeParams> {
		public:
			void operator()(const StateChangeInfo& changeInfo) const {
				const_cast<MockStateChange*>(this)->push(changeInfo);
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

		void SetBlockHeight(model::Block& block, Height height) {
			block.Timestamp = Timestamp{height.unwrap() * 1000};
			block.CumulativeDifficulty = Difficulty{height.unwrap()};
			block.Height = height;
		}

		config::LocalNodeConfiguration CreateUninitializedLocalNodeConfiguration() {
			auto blockChainConfig = model::BlockChainConfiguration::Uninitialized();
			blockChainConfig.MaxRollbackBlocks = Max_Rollback_Blocks;
			blockChainConfig.EffectiveBalanceRange = Effective_Balance;

			return config::LocalNodeConfiguration(
					std::move(blockChainConfig),
					config::NodeConfiguration::Uninitialized(),
					config::LoggingConfiguration::Uninitialized(),
					config::UserConfiguration::Uninitialized()
			);
		}

		struct ConsumerTestContext {
		public:
			ConsumerTestContext()
					: LocalNodeStateRef(
							*test::LocalNodeStateUtils::CreateLocalNodeState(
								CreateUninitializedLocalNodeConfiguration(),
								test::CreateCatapultCacheWithMarkerAccount()
							)
					)
			{
				BlockChainSyncHandlers handlers;
				handlers.DifficultyChecker = [this](const auto& remoteBlock, const auto& localBlock) {
					return DifficultyChecker(remoteBlock, localBlock);
				};
				handlers.UndoBlock = [this](const auto& block, const auto& state) {
					return UndoBlock(block, state);
				};
				handlers.StateChange = [this](const auto& changeInfo) {
					return StateChange(changeInfo);
				};
				handlers.TransactionsChange = [this](const auto& changeInfo) {
					return TransactionsChange(changeInfo);
				};

				auto process = [this](auto& state, auto& elements) {
					return Processor(state, elements);
				};
				Consumer = CreateMockBlockChainSyncConsumer(LocalNodeStateRef, handlers, process);
			}

		public:
			extensions::LocalNodeStateRef LocalNodeStateRef;
			std::vector<std::shared_ptr<model::Block>> OriginalBlocks; // original stored blocks (excluding nemesis)

			MockDifficultyChecker DifficultyChecker;
			MockUndoBlock UndoBlock;
			MockProcessor Processor;
			MockStateChange StateChange;
			MockTransactionsChange TransactionsChange;

			disruptor::DisruptorConsumer Consumer;

		public:
			void seedStorage(Height desiredHeight, size_t numTransactionsPerBlock = 0) {
				// Arrange:
				auto height = LocalNodeStateRef.Storage.view().chainHeight();
				auto storageModifier = LocalNodeStateRef.Storage.modifier();

				while (height < desiredHeight) {
					height = height + Height(1);

					auto transactions = test::GenerateRandomTransactions(numTransactionsPerBlock);
					auto pBlock = test::GenerateRandomBlockWithTransactions(transactions);
					SetBlockHeight(*pBlock, height);

					// - seed with random tx hashes
					auto blockElement = test::BlockToBlockElement(*pBlock);
					for (auto& transactionElement : blockElement.Transactions)
						transactionElement.EntityHash = test::GenerateRandomData<Hash256_Size>();

					storageModifier.saveBlock(blockElement);
					OriginalBlocks.push_back(std::move(pBlock));
				}
			}

		public:
			void assertDifficultyCheckerInvocation(const ConsumerInput& input) {
				// Assert:
				ASSERT_EQ(1u, DifficultyChecker.params().size());
				auto difficultyParams = DifficultyChecker.params()[0];

				ASSERT_EQ(input.blocks()[input.blocks().size() - 1].Block, *difficultyParams.RemoteBlock);
				ASSERT_EQ(*OriginalBlocks[OriginalBlocks.size() - 1], *difficultyParams.LocalBlock);
			}

			void assertUnwind(const std::vector<Height>& unwoundHeights) {
				// Assert:
				ASSERT_EQ(unwoundHeights.size(), UndoBlock.params().size());
				auto i = 0u;
				for (auto height : unwoundHeights) {
					const auto& undoBlockParams = UndoBlock.params()[i];

					EXPECT_EQ(*OriginalBlocks[(height - Height(2)).unwrap()], *undoBlockParams.pBlock) << "undo at " << i;
					EXPECT_TRUE(undoBlockParams.IsPassedMarkedCache) << "undo at " << i;
					++i;
				}
			}

			void assertProcessorInvocation(const ConsumerInput& input) {
				// Assert:
				ASSERT_EQ(1u, Processor.params().size());
				const auto& processorParams = Processor.params()[0];
				auto pCommonBlockElement = LocalNodeStateRef.Storage.view().loadBlockElement(input.blocks()[0].Block.Height - Height(1));

				EXPECT_EQ(pCommonBlockElement->Block, *processorParams.pParentBlock);
				EXPECT_EQ(pCommonBlockElement->EntityHash, processorParams.ParentHash);
				EXPECT_EQ(&input.blocks(), processorParams.pElements);
				EXPECT_TRUE(processorParams.IsPassedMarkedCache);
			}

			void assertNoStorageChanges() {
				// Assert: all original blocks are present in the storage
				auto storageView = LocalNodeStateRef.Storage.view();
				ASSERT_EQ(Height(OriginalBlocks.size()) + Height(1), storageView.chainHeight());
				for (const auto& pBlock : OriginalBlocks) {
					auto pStorageBlock = storageView.loadBlock(pBlock->Height);
					EXPECT_EQ(*pBlock, *pStorageBlock) << "at height " << pBlock->Height;
				}

				// - the cache was not committed
				EXPECT_FALSE(LocalNodeStateRef.CurrentCache.sub<cache::AccountStateCache>().createView()->contains(Sentinel_Processor_Public_Key));

				// - no state changes were announced
				EXPECT_EQ(0u, StateChange.params().size());

				// - no transaction changes were announced
				EXPECT_EQ(0u, TransactionsChange.params().size());
			}

			void assertStored(const ConsumerInput& input) {
				// Assert: all input blocks should be saved in the storage
				auto storageView = LocalNodeStateRef.Storage.view();
				auto inputHeight = input.blocks()[0].Block.Height;
				auto chainHeight = storageView.chainHeight();
				ASSERT_EQ(inputHeight + Height(input.blocks().size() - 1), chainHeight);
				for (auto height = inputHeight; height <= chainHeight; height = height + Height(1)) {
					auto pStorageBlock = storageView.loadBlock(height);
					EXPECT_EQ(input.blocks()[(height - inputHeight).unwrap()].Block, *pStorageBlock)
							<< "at height " << height;
				}

				// - non conflicting original blocks should still be in storage
				for (auto height = Height(2); height < inputHeight; height = height + Height(1)) {
					auto pStorageBlock = storageView.loadBlock(height);
					EXPECT_EQ(*OriginalBlocks[(height - Height(2)).unwrap()], *pStorageBlock) << "at height " << height;
				}

				// - the cache was committed (add 1 to OriginalBlocks.size() because it does not include the nemesis)
				EXPECT_TRUE(LocalNodeStateRef.CurrentCache.sub<cache::AccountStateCache>().createView()->contains(Sentinel_Processor_Public_Key));
				EXPECT_EQ(chainHeight, LocalNodeStateRef.CurrentCache.createView().height());

				// - the state was changed
				ASSERT_EQ(1u, StateChange.params().size());
				const auto& stateChangeParams = StateChange.params()[0];
				EXPECT_TRUE(stateChangeParams.IsPassedMarkedCache);
				EXPECT_EQ(chainHeight, stateChangeParams.Height);

				// - transaction changes were announced
				EXPECT_EQ(1u, TransactionsChange.params().size());
			}
		};
	}

	TEST(TEST_CLASS, CanProcessZeroEntities) {
		// Arrange:
		ConsumerTestContext context;

		// Assert:
		test::AssertPassthroughForEmptyInput(context.Consumer);
	}

	namespace {
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

	// region height check

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

	// region chain difficulty test

	TEST(TEST_CLASS, ChainWithSmallerDifficultyIsRejected) {
		// Arrange: create a local storage with blocks 1-7 and a remote storage with blocks 5-6
		//          (note that the test setup ensures difficulties are linearly correlated with number of blocks)
		ConsumerTestContext context;
		context.seedStorage(Height(7));
		auto input = CreateInput(Height(5), 2);

		auto remoteDifficulty = input.blocks()[input.blocks().size() - 1].Block.CumulativeDifficulty;
		auto localDifficulty = context.LocalNodeStateRef.Storage.view().loadBlock(Height(7))->CumulativeDifficulty;
		context.DifficultyChecker.setResult(remoteDifficulty > localDifficulty);

		// Act:
		auto result = context.Consumer(input);

		// Assert:
		test::AssertAborted(result, Failure_Consumer_Remote_Chain_Difficulty_Not_Better);
		EXPECT_EQ(0u, context.UndoBlock.params().size());
		EXPECT_EQ(0u, context.Processor.params().size());
		context.assertDifficultyCheckerInvocation(input);
		context.assertNoStorageChanges();
	}

	TEST(TEST_CLASS, ChainWithIdenticalDifficultyIsRejected) {
		// Arrange: create a local storage with blocks 1-7 and a remote storage with blocks 6-7
		//          (note that the test setup ensures difficulties are linearly correlated with number of blocks)
		ConsumerTestContext context;
		context.seedStorage(Height(7));
		auto input = CreateInput(Height(6), 2);

		auto remoteDifficulty = input.blocks()[input.blocks().size() - 1].Block.CumulativeDifficulty;
		auto localDifficulty = context.LocalNodeStateRef.Storage.view().loadBlock(Height(7))->CumulativeDifficulty;
		context.DifficultyChecker.setResult(remoteDifficulty > localDifficulty);

		// Act:
		auto result = context.Consumer(input);

		// Assert:
		test::AssertAborted(result, Failure_Consumer_Remote_Chain_Difficulty_Not_Better);
		EXPECT_EQ(0u, context.UndoBlock.params().size());
		EXPECT_EQ(0u, context.Processor.params().size());
		context.assertDifficultyCheckerInvocation(input);
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
		context.assertStored(input);
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
		EXPECT_EQ(3u, context.UndoBlock.params().size());
		context.assertDifficultyCheckerInvocation(input);
		context.assertUnwind({ Height(7), Height(6), Height(5) });
		context.assertProcessorInvocation(input);
		context.assertStored(input);
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
		EXPECT_EQ(1u, context.UndoBlock.params().size());
		context.assertDifficultyCheckerInvocation(input);
		context.assertUnwind({ Height(7) });
		context.assertProcessorInvocation(input);
		context.assertStored(input);
	}

	TEST(TEST_CLASS, CanSyncIncompatibleChainsWhereShorterRemoteChainHasHigherDifficulty) {
		// Arrange: create a local storage with blocks 1-7 and a remote storage with blocks 5
		ConsumerTestContext context;
		context.seedStorage(Height(7));
		auto input = CreateInput(Height(5), 1);
		const_cast<model::Block&>(input.blocks()[0].Block).CumulativeDifficulty = Difficulty(Base_Difficulty * 3);

		// Act:
		auto result = context.Consumer(input);

		// Assert:
		test::AssertContinued(result);
		EXPECT_EQ(3u, context.UndoBlock.params().size());
		context.assertDifficultyCheckerInvocation(input);
		context.assertUnwind({ Height(7), Height(6), Height(5) });
		context.assertProcessorInvocation(input);
		context.assertStored(input);
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
					add(elementIndex, test::GenerateRandomTransaction(), test::GenerateRandomData<Hash256_Size>());
			}

			void addFromStorage(size_t elementIndex, const io::BlockStorageCache& storage, Height height, size_t txIndex) {
				auto pBlockElement = storage.view().loadBlockElement(height);

				auto i = 0u;
				for (const auto& transactionElement : pBlockElement->Transactions) {
					if (i++ != txIndex)
						continue;

					add(elementIndex, test::CopyTransaction(transactionElement.Transaction), transactionElement.EntityHash);
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
		context.assertStored(input);

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
		auto expectedRevertedHashes = ExtractTransactionHashesFromStorage(context.LocalNodeStateRef.Storage.view(), Height(5), Height(7));

		// Act:
		auto result = context.Consumer(input);

		// Assert:
		test::AssertContinued(result);
		EXPECT_EQ(3u, context.UndoBlock.params().size());
		context.assertDifficultyCheckerInvocation(input);
		context.assertUnwind({ Height(7), Height(6), Height(5) });
		context.assertProcessorInvocation(input);
		context.assertStored(input);

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
		builder.addFromStorage(2, context.LocalNodeStateRef.Storage, Height(5), 2);
		builder.addFromStorage(0, context.LocalNodeStateRef.Storage, Height(7), 1);

		// - extract original hashes from storage
		auto expectedRevertedHashes = ExtractTransactionHashesFromStorage(context.LocalNodeStateRef.Storage.view(), Height(5), Height(7));
		expectedRevertedHashes.erase(expectedRevertedHashes.begin() + 2 * 3 + 1); // block 7 tx 2
		expectedRevertedHashes.erase(expectedRevertedHashes.begin() + 2); // block 5 tx 3

		// Act:
		auto result = context.Consumer(input);

		// Assert:
		test::AssertContinued(result);
		EXPECT_EQ(3u, context.UndoBlock.params().size());
		context.assertDifficultyCheckerInvocation(input);
		context.assertUnwind({ Height(7), Height(6), Height(5) });
		context.assertProcessorInvocation(input);
		context.assertStored(input);

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
			Hash256 expectedGenerationHash{ { i++ } };
			EXPECT_EQ(expectedGenerationHash, blockElement.GenerationHash) << "generation hash at " << i;
		}
	}

	// endregion
}}
