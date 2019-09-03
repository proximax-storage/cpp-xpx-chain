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

#include "catapult/cache/CatapultCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/ImportanceView.h"
#include "catapult/chain/BlockExecutor.h"
#include "catapult/extensions/PluginUtils.h"
#include "catapult/observers/NotificationObserverAdapter.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/ResolverTestUtils.h"
#include "tests/test/core/mocks/MockTransaction.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"
#include "catapult/constants.h"

namespace catapult { namespace extensions {

#define TEST_CLASS RootObserverIntegrityTests

	namespace {
		using NotifyMode = observers::NotifyMode;

		uint64_t Importance_Grouping = 10;

		constexpr auto Harvesting_Mosaic_Id = MosaicId(9876);

		Importance GetTotalChainImportance(uint32_t numAccounts) {
			return Importance(numAccounts * (numAccounts + 1) / 2);
		}

		Amount GetTotalChainBalance(uint32_t numAccounts) {
			return Amount(GetTotalChainImportance(numAccounts).unwrap() * 1'000'000);
		}

		auto CreateConfiguration(uint32_t numAccounts) {
			test::MutableBlockchainConfiguration config;
			config.Immutable.NetworkIdentifier = model::NetworkIdentifier::Mijin_Test;
			config.Immutable.HarvestingMosaicId = Harvesting_Mosaic_Id;
			config.Network.MaxDifficultyBlocks = 4;
			config.Network.MaxRollbackBlocks = 124;
			config.Network.BlockPruneInterval = 360;
			config.Network.TotalChainImportance = GetTotalChainImportance(numAccounts);
			config.Network.MinHarvesterBalance = Amount(1'000'000);
			config.Network.ImportanceGrouping = Importance_Grouping;
			return config.ToConst();
		}

		class TestContext {
		public:
			explicit TestContext(uint32_t numAccounts)
					: m_config(CreateConfiguration(numAccounts))
					, m_pPluginManager(test::CreatePluginManagerWithRealPlugins(m_config))
					, m_cache(m_pPluginManager->createCache())
					, m_specialAccountKey(test::GenerateRandomByteArray<Key>()) {
				// register mock transaction plugin so that BalanceTransferNotifications are produced and observed
				// (MockTransaction Publish XORs recipient address, so XOR address resolver is required
				// for proper roundtripping or else test will fail)
				m_pPluginManager->addTransactionSupport(mocks::CreateMockTransactionPlugin(mocks::PluginOptionFlags::Publish_Transfers));

				initNemesisCache(m_cache, numAccounts);
			}

		public:
			// accounts are funded by "nemesis" account (startAccountId with 1M * baseUnit, startAccountId + 1 with 2M * baseUnit, ...)
			void addAccounts(uint8_t startAccountId, uint8_t numAccounts, Height height, uint8_t baseUnit = 1) {
				auto& transactions = m_heightToTransactions[height];

				for (uint8_t i = startAccountId; i < startAccountId + numAccounts; ++i) {
					uint8_t multiplier = i - startAccountId + 1;
					auto pTransaction = mocks::CreateTransactionWithFeeAndTransfers(Amount(), {
						{ test::UnresolveXor(Harvesting_Mosaic_Id), Amount(multiplier * baseUnit * 1'000'000) }
					});
					pTransaction->Signer = m_specialAccountKey;
					pTransaction->Recipient = Key{ { i } };
					transactions.push_back(std::move(pTransaction));
				}
			}

			// send entire balance of all accounts to "nemesis" account
			void zeroBalances(uint8_t startAccountId, uint8_t numAccounts, Height height) {
				auto& transactions = m_heightToTransactions[height];

				auto accountStateCacheView = m_cache.sub<cache::AccountStateCache>().createView(Height{0});
				for (uint8_t i = startAccountId; i < startAccountId + numAccounts; ++i) {
					const auto& accountState = accountStateCacheView->find(Key{ { i } }).get();
					auto pTransaction = mocks::CreateTransactionWithFeeAndTransfers(Amount(), {
						{ test::UnresolveXor(Harvesting_Mosaic_Id), accountState.Balances.get(Harvesting_Mosaic_Id) }
					});
					pTransaction->Signer = Key{ { i } };
					pTransaction->Recipient = m_specialAccountKey;
					transactions.push_back(std::move(pTransaction));
				}
			}

			// send entire balance of accountId1 to accountId2
			void moveBalance(uint8_t accountId1, uint8_t accountId2, Height height) {
				auto& transactions = m_heightToTransactions[height];

				auto accountStateCacheView = m_cache.sub<cache::AccountStateCache>().createView(Height{0});
				const auto& accountState1 = accountStateCacheView->find(Key{ { accountId1 } }).get();
				auto pTransaction = mocks::CreateTransactionWithFeeAndTransfers(Amount(), {
					{ test::UnresolveXor(Harvesting_Mosaic_Id), accountState1.Balances.get(Harvesting_Mosaic_Id) }
				});
				pTransaction->Signer = accountState1.PublicKey;
				pTransaction->Recipient = Key{ { accountId2 } };
				transactions.push_back(std::move(pTransaction));
			}

		public:
			void notify(Height height, NotifyMode mode) {
				// Arrange:
				auto pBlock = createBlock(height);
				auto blockElement = test::BlockToBlockElement(*pBlock);

				// - use XOR resolvers because Publish_Transfers is enabled
				observers::NotificationObserverAdapter rootObserver(
						m_pPluginManager->createObserver(),
						m_pPluginManager->createNotificationPublisher());
				auto resolverContext = test::CreateResolverContextXor();

				auto delta = m_cache.createDelta();
				auto observerState = observers::ObserverState(delta, m_state);
				auto blockExecutionContext = chain::BlockExecutionContext(rootObserver, resolverContext, observerState);

				// Act: use BlockExecutor to execute all transactions and blocks
				if (NotifyMode::Commit == mode) {
					chain::ExecuteBlock(blockElement, blockExecutionContext);
					m_cache.commit(height);
				} else {
					chain::RollbackBlock(blockElement, blockExecutionContext);
					m_cache.commit(Height(height.unwrap() - 1));
				}
			}

			void notifyAllCommit(Height startHeight, Height endHeight) {
				for (auto height = startHeight; height <= endHeight; height = height + Height(1))
					notify(height, NotifyMode::Commit);
			}

			void notifyAllRollback(Height startHeight, Height endHeight) {
				for (auto height = startHeight; height >= endHeight; height = height - Height(1))
					notify(height, NotifyMode::Rollback);
			}

		public:
			struct AssertOptions {
			public:
				AssertOptions(
						uint8_t startAccountId,
						uint8_t numAccounts,
						uint64_t height,
						uint8_t startAdjustment = 0,
						uint8_t multiplier = 1)
						: StartAccountId(startAccountId)
						, NumAccounts(numAccounts)
						, Height(height)
						, StartAdjustment(startAdjustment)
						, Multiplier(multiplier)
				{}

			public:
				uint8_t StartAccountId;
				uint8_t NumAccounts;
				uint64_t Height;
				uint8_t StartAdjustment;
				uint8_t Multiplier;
			};

			void assertRemovedAccounts(uint8_t startAccountId, uint8_t numAccounts) {
				auto accountStateCacheView = m_cache.sub<cache::AccountStateCache>().createView(Height{0});
				for (uint8_t i = startAccountId; i < startAccountId + numAccounts; ++i)
					EXPECT_FALSE(accountStateCacheView->contains(Key{ { i } })) << "balance for account " << static_cast<int>(i);
			}

			void assertSingleEffectiveBalance(
					uint8_t accountId,
					Amount expectedBalance,
					Height height) {
				const auto message = "balance for account " + std::to_string(accountId);

				auto cacheView = m_cache.createView();
				EXPECT_EQ(height, cacheView.height()) << "Height is not equil";

				cache::ReadOnlyAccountStateCache readOnly(cacheView.sub<cache::AccountStateCache>());
				cache::ImportanceView importanceView(readOnly);
				EXPECT_EQ(expectedBalance.unwrap(), importanceView.getAccountImportanceOrDefault({ { accountId } }, cacheView.height()).unwrap()) << message;
			}

			void assertLinearEffectiveBalance(const AssertOptions& options) {
				for (uint8_t i = options.StartAccountId; i < options.StartAccountId + options.NumAccounts; ++i) {
					uint64_t multiplier = options.StartAdjustment + i - options.StartAccountId + 1;
					assertSingleEffectiveBalance(i, Amount(multiplier * options.Multiplier * 1'000'000), Height(options.Height));
				}
			}

		private:
			void inline initNemesisCache(cache::CatapultCache& cache, const uint32_t& numAccounts) {
				// seed the "nemesis" / transfer account (this account is used to fund all other accounts)
				auto delta = cache.createDelta();
				auto& accountStateCache = delta.sub<cache::AccountStateCache>();
				accountStateCache.addAccount(m_specialAccountKey, Height(1));
				auto& accountState = accountStateCache.find(m_specialAccountKey).get();
				accountState.Balances.track(Harvesting_Mosaic_Id);
				accountState.Balances.credit(Harvesting_Mosaic_Id, GetTotalChainBalance(numAccounts), Height(1));
				cache.commit(Height());
			}

			std::unique_ptr<model::Block> createBlock(Height height) {
				// if there are transactions, add them to the block
				auto transactionsIter = m_heightToTransactions.find(height);
				auto pBlock = m_heightToTransactions.end() == transactionsIter
						? test::GenerateEmptyRandomBlock()
						: test::GenerateBlockWithTransactions(transactionsIter->second);
				pBlock->Height = height;
				pBlock->FeeMultiplier = BlockFeeMultiplier(0);
				pBlock->FeeInterest = 1;
				pBlock->FeeInterestDenominator = 1;

				// in order to emulate correctly, block must have same signer when executed and reverted
				auto signerIter = m_heightToBlockSigner.find(height);
				if (m_heightToBlockSigner.cend() == signerIter)
					m_heightToBlockSigner.emplace(height, pBlock->Signer); // save signer used during commit
				else
					pBlock->Signer = signerIter->second;

				return pBlock;
			}

		private:
			config::BlockchainConfiguration m_config;
			std::shared_ptr<plugins::PluginManager> m_pPluginManager;
			cache::CatapultCache m_cache;
			state::CatapultState m_state;

			Key m_specialAccountKey;

			// undo tests require same block signer at heights (because HarvestFeeObserver needs to debit an existing account)
			std::unordered_map<Height, Key, utils::BaseValueHasher<Height>> m_heightToBlockSigner;
			std::unordered_map<Height, test::MutableTransactions, utils::BaseValueHasher<Height>> m_heightToTransactions;
		};
	}

	// region execute

	TEST(TEST_CLASS, ExecuteCalculatesEffectiveBalanceCorrectly) {
		// Arrange: create a context with 10 accounts
		TestContext context(10);
		context.addAccounts(1, 10, Height(2));

		// Act: commit second block to update balance
		context.notifyAllCommit(Height(2), Height(2 + Importance_Grouping));

		// Assert: balance of account must be updated
		context.assertLinearEffectiveBalance({ 1, 10, 2 + Importance_Grouping}); // updated
	}

	TEST(TEST_CLASS, ExecuteCalculatesEffectiveBalanceCorrectly_AfterZeroBalanceOfAllAccounts) {
		// Arrange: create a context with 10 accounts
		TestContext context(10);
		context.addAccounts(1, 10, Height(2));
		context.notify(Height(2), NotifyMode::Commit);

		context.zeroBalances(1, 10, Height(2 + Importance_Grouping + 1));
		context.notifyAllCommit(Height(3), Height(2 + Importance_Grouping));

		// Assert: effective balance must be not zero on height 2 + Importance_Grouping
		context.assertLinearEffectiveBalance({1, 10, 2 + Importance_Grouping });

		// But effective balance must be zero on 2 + Importance_Grouping + 1 height
		context.notify(Height(2 + Importance_Grouping + 1), NotifyMode::Commit);
		context.assertLinearEffectiveBalance({1, 10, 2 + Importance_Grouping + 1, 0, 0});
	}

	TEST(TEST_CLASS, ExecuteCalculatesEffectiveBalanceCorrectly_WithDifferentBalances) {
		// Arrange: create a context with 10 accounts
		TestContext context(10);
		context.addAccounts(1, 10, Height(2));
		context.notify(Height(2), NotifyMode::Commit);

		// - replace some existing balance accounts with new balance accounts
		context.zeroBalances(1, 5, Height(3));
		context.addAccounts(25, 5, Height(3));
		context.addAccounts(30, 5, Height(3), 0);

		context.notifyAllCommit(Height(3), Height(2 + Importance_Grouping + 1));

		// Assert:
		context.assertLinearEffectiveBalance({ 1, 5, 2 + Importance_Grouping + 1, 0, 0 });
		context.assertLinearEffectiveBalance({ 6, 5, 2 + Importance_Grouping + 1, 5 });
		context.assertLinearEffectiveBalance({ 25, 5, 2 + Importance_Grouping + 1 });
		context.assertLinearEffectiveBalance({ 30, 5, 2 + Importance_Grouping + 1, 0, 0 });
	}

	// endregion

	// region undo one level
	TEST(TEST_CLASS, UndoCalculatesEffectiveBalanceCorrectly) {
		// Arrange: create a context with 10 accounts
		TestContext context(10);
		context.addAccounts(1, 10, Height(2));

		// Act: commit second block to update balance
		context.notifyAllCommit(Height(2), Height(2 + Importance_Grouping));

		// Assert: balance of account must be updated
		context.assertLinearEffectiveBalance({ 1, 10, 2 + Importance_Grouping }); // updated

		// Act: undo second block to update balance
		context.notify(Height(2 + Importance_Grouping), NotifyMode::Rollback);

		context.assertLinearEffectiveBalance({ 1, 10, 2 + Importance_Grouping - 1, 0, 0 }); // updated
	}

	TEST(TEST_CLASS, UndoCalculatesEffectiveBalanceCorrectly_AfterZeroBalanceOfAllAccounts) {
		// Arrange: create a context with 10 accounts
		TestContext context(10);
		context.addAccounts(1, 10, Height(2));
		context.notify(Height(2), NotifyMode::Commit);

		context.zeroBalances(1, 10, Height(2 + Importance_Grouping + 1));

		context.notifyAllCommit(Height(3), Height(2 + Importance_Grouping));

		// Assert: effective balance must be not zero on height 2 + Importance_Grouping
		context.assertLinearEffectiveBalance({1, 10, 2 + Importance_Grouping});

		// But effective balance must be zero on 2 + Importance_Grouping + 1 height
		context.notify(Height(2 + Importance_Grouping + 1), NotifyMode::Commit);
		context.assertLinearEffectiveBalance({1, 10, 2 + Importance_Grouping + 1, 0, 0});

		// Let's undo last block
		context.notify(Height(2 + Importance_Grouping + 1), NotifyMode::Rollback);
		context.assertLinearEffectiveBalance({1, 10, 2 + Importance_Grouping });
	}

	TEST(TEST_CLASS, UndoCalculatesEffectiveBalanceCorrectly_UndoMultiplyBlocks) {
		// Arrange: create a context with 10/10 important accounts
		TestContext context(30);
		context.addAccounts(1, 10, Height(2));
		context.addAccounts(1, 10, Height(2 + Importance_Grouping));
		context.addAccounts(1, 10, Height(2 + 2 * Importance_Grouping));

		context.notifyAllCommit(Height(2), Height(2 + Importance_Grouping));
		context.assertLinearEffectiveBalance({ 1, 10, 2 + Importance_Grouping, 0, 1 });

		context.notifyAllCommit(Height(2 + Importance_Grouping + 1), Height(2 + 2 * Importance_Grouping));
		context.assertLinearEffectiveBalance({ 1, 10, 2 + 2 * Importance_Grouping, 0, 2});

		context.notifyAllCommit(Height(2 + 2 * Importance_Grouping + 1), Height(2 + 3 * Importance_Grouping));
		context.assertLinearEffectiveBalance({ 1, 10, 2 + 3 * Importance_Grouping, 0, 3});

		// Act: let's undo blocks
		context.notifyAllRollback(Height(2 + 3 * Importance_Grouping), Height(2 + 2 * Importance_Grouping + 1));
		context.assertLinearEffectiveBalance({ 1, 10, 2 + 2 * Importance_Grouping, 0, 2});

		context.notifyAllRollback(Height(2 + 2 * Importance_Grouping), Height(2 + 1 * Importance_Grouping + 1));
		context.assertLinearEffectiveBalance({ 1, 10, 2 + 1 * Importance_Grouping, 0, 1});

		context.notifyAllRollback(Height(2 + Importance_Grouping), Height(2));
		context.assertLinearEffectiveBalance({ 1, 10, 1, 0, 0});
	}

	// endregion
}}
