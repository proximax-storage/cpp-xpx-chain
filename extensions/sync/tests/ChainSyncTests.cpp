/**
*** Copyright (c) 2018-present,
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

#include "catapult/api/LocalChainApi.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/chain/BlockDifficultyScorer.h"
#include "catapult/chain/BlockExecutor.h"
#include "catapult/chain/ChainResults.h"
#include "catapult/consumers/ConsumerResults.h"
#include "catapult/model/Address.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/observers/NotificationObserverAdapter.h"
#include "extensions/sync/src/DispatcherService.h"
#include "MockRemoteChainApi.h"
#include "sdk/src/builders/TransferBuilder.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "sdk/src/extensions/TransactionExtensions.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/local/ServiceLocatorTestContext.h"
#include "tests/test/nodeps/MijinConstants.h"
#include "tests/test/nodeps/TestConstants.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"

namespace catapult { namespace sync {

#define TEST_CLASS ChainSyncTests

	namespace {
		auto Special_Account_Key_Pair = crypto::KeyPair::FromString(test::Mijin_Test_Private_Keys[0]);
		auto Nemesis_Account_Key_Pair = crypto::KeyPair::FromString(test::Mijin_Test_Private_Keys[1]);
		uint64_t constexpr Initial_Balance = 1'000'000'000'000'000u;

		chain::ChainSynchronizerConfiguration CreateChainSynchronizerConfiguration(
				const config::BlockchainConfiguration& config) {
			chain::ChainSynchronizerConfiguration chainSynchronizerConfig;
			chainSynchronizerConfig.MaxBlocksPerSyncAttempt = config.Node.MaxBlocksPerSyncAttempt;
			chainSynchronizerConfig.MaxChainBytesPerSyncAttempt = config.Node.MaxChainBytesPerSyncAttempt.bytes32();
			return chainSynchronizerConfig;
		}

		config::BlockchainConfiguration CreateBlockchainConfiguration() {
			test::MutableBlockchainConfiguration config;

			config.Immutable.NetworkIdentifier = model::NetworkIdentifier::Mijin_Test;

			config.Network.MaxRollbackBlocks = 5u;
			config.Network.ImportanceGrouping = 10u;
			config.Network.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(15u);
			config.Network.BlockTimeSmoothingFactor = 3000u;
			config.Network.MaxBlockFutureTime = utils::TimeSpan::FromSeconds(10u);
			config.Network.MaxDifficultyBlocks = 4u;
			config.Network.BlockPruneInterval = 360u;
			config.Network.GreedDelta = 0.5;
			config.Network.GreedExponent = 2.0;
			config.Network.Plugins.emplace(PLUGIN_NAME(transfer), utils::ConfigurationBag({{ "", { { "maxMessageSize", "0" }, { "maxMosaicsSize", "512" } } }}));

			config.Node.MaxBlocksPerSyncAttempt = 30u;
			config.Node.MaxChainBytesPerSyncAttempt = utils::FileSize::FromMegabytes(1u);
			config.Node.BlockDisruptorSize = 4096u;
			config.Node.TransactionDisruptorSize = 16384u;
			config.Node.FeeInterest = 1u;
			config.Node.FeeInterestDenominator = 1u;

			config.SupportedEntityVersions = test::CreateSupportedEntityVersions();

			return config.ToConst();
		}

		struct DispatcherServiceTraits {
			static constexpr auto CreateRegistrar = CreateDispatcherServiceRegistrar;
		};

		class TestContext : public test::ServiceLocatorTestContext<DispatcherServiceTraits> {
		private:
			using BaseType = test::ServiceLocatorTestContext<DispatcherServiceTraits>;

		public:
			TestContext(const config::BlockchainConfiguration& config, uint64_t initialBalance)
				: BaseType(test::CreateEmptyCatapultCache<test::CoreSystemCacheFactory>(config), config) {

				testState().loadPluginByName("", "catapult.coresystem");
				for (const auto& pair : config.Network.Plugins)
					testState().loadPluginByName("", pair.first);

				auto modifier = testState().state().storage().modifier();
				modifier.dropBlocksAfter(Height{0u});
				modifier.commit();

				initializeCache(initialBalance);
			}

			void initializeCache(uint64_t initialBalance) {
				auto delta = testState().state().cache().createDelta();

				auto& accountCache = delta.sub<cache::AccountStateCache>();
				accountCache.addAccount(Special_Account_Key_Pair.publicKey(), Height{1});
				auto& specialAccountState = accountCache.find(Special_Account_Key_Pair.publicKey()).get();
				specialAccountState.Balances.track(test::Default_Currency_Mosaic_Id);
				specialAccountState.Balances.credit(test::Default_Currency_Mosaic_Id, Amount(initialBalance), Height{1});
				accountCache.addAccount(Nemesis_Account_Key_Pair.publicKey(), Height{1});
				auto& nemesisAccountState = accountCache.find(Nemesis_Account_Key_Pair.publicKey()).get();
				nemesisAccountState.Balances.track(test::Default_Currency_Mosaic_Id);
				nemesisAccountState.Balances.credit(test::Default_Currency_Mosaic_Id, Amount(initialBalance), Height{1});

				testState().state().cache().commit(Height{1});
			}

			void executeBlock(const model::BlockElement& blockElement) {
				auto& state = testState().state();
				auto delta = state.cache().createDelta();
				auto observerState = observers::ObserverState(delta, state.state());

				observers::NotificationObserverAdapter rootObserver(
						state.pluginManager().createObserver(),
						state.pluginManager().createNotificationPublisher());
				auto resolverContext = model::ResolverContext();
				auto blockExecutionContext = chain::BlockExecutionContext(rootObserver, resolverContext, state.pluginManager().configHolder(), observerState);

				chain::ExecuteBlock(blockElement, blockExecutionContext);
				state.cache().commit(blockElement.Block.Height);

				auto modifier = testState().state().storage().modifier();
				modifier.saveBlock(blockElement);
				modifier.commit();
			}

			model::BlockElement createBlock(
					const Height& height,
					const Timestamp& timestamp,
					const model::PreviousBlockContext& previousBlockContext,
					const crypto::KeyPair& signer,
					const test::ConstTransactions& transactions) {
				m_pLastBlock = model::CreateBlock(previousBlockContext, model::NetworkIdentifier::Mijin_Test,
						signer.publicKey(), transactions);
				m_pLastBlock->Height = height;
				m_pLastBlock->Timestamp = timestamp;
				m_pLastBlock->FeeInterest = 1;
				m_pLastBlock->FeeInterestDenominator = 1;
				if (height.unwrap() == 1u)
					m_pLastBlock->Difficulty = Difficulty{1000u};
				else
					chain::TryCalculateDifficulty(testState().state().cache().sub<cache::BlockDifficultyCache>(),
						state::BlockDifficultyInfo(height, timestamp, m_pLastBlock->Difficulty),
						testState().state().config().Network, m_pLastBlock->Difficulty);
				SignBlockHeader(signer, *m_pLastBlock);

				auto blockElement = test::BlockToBlockElement(*m_pLastBlock);
				blockElement.GenerationHash = model::CalculateGenerationHash(
					previousBlockContext.GenerationHash,
					signer.publicKey());

				return blockElement;
			}

			void createBlocks(
					const Height& endHeight,
					const Timestamp& blockTime,
					const model::PreviousBlockContext& lastBlockContext,
					const crypto::KeyPair& signer,
					const test::ConstTransactions& startBlockTransactions) {
				model::PreviousBlockContext previousBlockContext{lastBlockContext};
				auto startHeight = previousBlockContext.BlockHeight + Height{1u};
				for (auto height = startHeight; height <= endHeight; height = height + Height{1u}) {
					auto timestamp = previousBlockContext.Timestamp + blockTime;
					auto blockElement = createBlock(height, timestamp, previousBlockContext, signer,
						(height == startHeight) ? startBlockTransactions : test::ConstTransactions{});
					executeBlock(blockElement);
					previousBlockContext = model::PreviousBlockContext{blockElement};
				}
			}

		private:
			model::UniqueEntityPtr<model::Block> m_pLastBlock;
		};

		model::PreviousBlockContext PopulateCommonBlocks(
				TestContext& localContext,
				TestContext& remoteContext,
				const Height& endHeight) {
			model::PreviousBlockContext previousBlockContext{};
			previousBlockContext.BlockHeight = Height{1};
			for (auto height = Height{1u}; height <= endHeight; height = height + Height{1u}) {
				auto timestamp = Timestamp{height.unwrap() *
					localContext.testState().state().config().Network.BlockGenerationTargetTime.millis()};
				const auto& signer = height.unwrap() % 2 ?
					Nemesis_Account_Key_Pair : Special_Account_Key_Pair;
				test::ConstTransactions transactions{};
				auto blockElement = localContext.createBlock(height, timestamp, previousBlockContext, signer, transactions);
				previousBlockContext = model::PreviousBlockContext{blockElement};

				localContext.executeBlock(blockElement);
				remoteContext.executeBlock(blockElement);
			}

			return previousBlockContext;
		}

		test::ConstTransactions CreateSpecialTransaction(model::NetworkIdentifier networkIdentifier, uint64_t initialBalance) {
			auto nemesisAccountAddress = model::PublicKeyToAddress(
				Nemesis_Account_Key_Pair.publicKey(),
				networkIdentifier);

			builders::TransferBuilder builder(
				networkIdentifier,
				Special_Account_Key_Pair.publicKey() /* signer */
			);
			builder.setRecipient(extensions::CopyToUnresolvedAddress(nemesisAccountAddress));
			builder.addMosaic(model::UnresolvedMosaic{ UnresolvedMosaicId(test::Default_Currency_Mosaic_Id.unwrap()), Amount(initialBalance / 2u) });

			test::ConstTransactions transactions{};
			auto pTransaction = builder.build();
			extensions::TransactionExtensions(test::GenerateRandomByteArray<GenerationHash>()).sign(Special_Account_Key_Pair, *pTransaction);
			transactions.push_back(std::move(pTransaction));

			return transactions;
		}

		ionet::NodeInteractionResultCode SynchronizeChains(
				TestContext& localContext,
				TestContext& remoteContext,
				disruptor::ConsumerCompletionResult& consumerResult) {
			auto& state = localContext.testState().state();

			auto blockRangeConsumer = state.hooks().completionAwareBlockRangeConsumerFactory()(disruptor::InputSource::Remote_Pull);
			auto blockRangeConsumerWrapper = [&blockRangeConsumer, &consumerResult](auto&& range, const auto& processingComplete) {
				auto processingCompleteWrapper = [processingComplete, &consumerResult](auto elementId, const auto& result) {
					consumerResult = result;
					processingComplete(elementId, result);
				};
				return blockRangeConsumer(std::move(range), processingCompleteWrapper);
			};

			auto chainSynchronizer = chain::CreateChainSynchronizer(
				api::CreateLocalChainApi(
					state.storage(),
					[]() { return model::ChainScore{1000u}; }),
				CreateChainSynchronizerConfiguration(state.config()),
				state,
				blockRangeConsumerWrapper);

			extensions::LocalNodeChainScore remoteChainScore{model::ChainScore{2000u}};
			mocks::MockRemoteChainApi remoteApi{
				test::GenerateKeyPair().publicKey(),
				remoteContext.testState().state().storage(),
				remoteChainScore};

			auto syncResult = chainSynchronizer(remoteApi).get();

			std::this_thread::sleep_for(std::chrono::milliseconds(100u));

			return syncResult;
		}

		ionet::NodeInteractionResultCode SynchronizeChains(
				const Height& commonHeight,
				const Height& localEndHeight,
				const Height& remoteEndHeight,
				uint64_t initialBalance,
				disruptor::ConsumerCompletionResult& consumerResult) {
			auto config = CreateBlockchainConfiguration();

			TestContext localContext(config, initialBalance);
			localContext.boot();

			TestContext remoteContext(config, initialBalance);
			remoteContext.boot();

			auto previousBlockContext = PopulateCommonBlocks(localContext, remoteContext, commonHeight);

			auto localTransactions = CreateSpecialTransaction(config.Immutable.NetworkIdentifier, initialBalance);
			localContext.createBlocks(
				localEndHeight,
				Timestamp(config.Network.BlockGenerationTargetTime.millis()),
				previousBlockContext,
				Nemesis_Account_Key_Pair,
				localTransactions);

			// Remote blocks have less timestamps, so remote chain score is greater.
			remoteContext.createBlocks(
				remoteEndHeight,
				Timestamp(config.Network.BlockGenerationTargetTime.millis() - 1000),
				previousBlockContext,
				Special_Account_Key_Pair,
				test::ConstTransactions());

			return SynchronizeChains(localContext, remoteContext, consumerResult);
		}
	}

	TEST(TEST_CLASS, DoubleSpend1) {
		// Arange:
		Height commonHeight{5};
		Height localEndHeight{9};
		Height remoteEndHeight{8};
		disruptor::ConsumerCompletionResult consumerResult;

		// Act:
		SynchronizeChains(commonHeight, localEndHeight, remoteEndHeight, Initial_Balance, consumerResult);

		// Assert:
		auto validationResult = static_cast<validators::ValidationResult>(consumerResult.CompletionCode);
		ASSERT_EQ(consumers::Failure_Consumer_Remote_Chain_Score_Not_Better, validationResult);
	}

	TEST(TEST_CLASS, DoubleSpend2) {
		// Arange:
		Height commonHeight{10};
		Height localEndHeight{11};
		Height remoteEndHeight{11};
		disruptor::ConsumerCompletionResult consumerResult;

		// Act:
		SynchronizeChains(commonHeight, localEndHeight, remoteEndHeight, Initial_Balance, consumerResult);

		// Assert:
		auto validationResult = static_cast<validators::ValidationResult>(consumerResult.CompletionCode);
		ASSERT_EQ(chain::Failure_Chain_Block_Not_Hit, validationResult);
	}

	TEST(TEST_CLASS, DoubleSpend3) {
		// Arange:
		Height commonHeight{50};
		Height localEndHeight{55};
		Height remoteEndHeight{55};
		disruptor::ConsumerCompletionResult consumerResult;

		// Act:
		SynchronizeChains(commonHeight, localEndHeight, remoteEndHeight, Initial_Balance, consumerResult);

		// Assert:
		auto validationResult = static_cast<validators::ValidationResult>(consumerResult.CompletionCode);
		ASSERT_EQ(chain::Failure_Chain_Block_Not_Hit, validationResult);
	}

	TEST(TEST_CLASS, DoubleSpend4) {
		// Arange:
		Height commonHeight{50};
		Height localEndHeight{100};
		Height remoteEndHeight{100};
		disruptor::ConsumerCompletionResult consumerResult;

		// Act:
		auto syncResult = SynchronizeChains(commonHeight, localEndHeight, remoteEndHeight, Initial_Balance, consumerResult);

		// Assert:
		ASSERT_EQ(ionet::NodeInteractionResultCode::Failure, syncResult);
	}

	TEST(TEST_CLASS, DoubleSpendWhenRemoteNodeSpentNotAllMoneyAndCanDoAttack) {
		// Arange:
		Height commonHeight{10};
		Height localEndHeight{11};
		Height remoteEndHeight{11};
		disruptor::ConsumerCompletionResult consumerResult;

		// Act:
		auto syncResult = SynchronizeChains(commonHeight, localEndHeight, remoteEndHeight, Initial_Balance * 10, consumerResult);

		// Assert:
		ASSERT_EQ(ionet::NodeInteractionResultCode::Success, syncResult);
	}
}}
