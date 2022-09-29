/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/LockFundTotalStakedReceipt.h"
#include "src/cache/LockFundCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "plugins/services/globalstore/src/cache/GlobalStoreCache.h"
#include "plugins/services/globalstore/src/state/BaseConverters.h"
#include "plugins/services/globalstore/src/state/GlobalEntry.h"
#include "tests/test/LockFundCacheFactory.h"
#include "tests/test/core/NotificationTestUtils.h"
#include "src/config/LockFundConfiguration.h"
#include "src/observers/Observers.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS LockFundBlockObserverTests

	using ObserverTestContext = test::ObserverTestContextT<test::LockFundCacheFactory>;

	DEFINE_COMMON_OBSERVER_TESTS(LockFundBlock,)

	namespace {

		auto PrepareDefaultAccount(const Key& signer, const Height& unlockHeight, const std::map<MosaicId, Amount>& mosaics = {{MosaicId(72), Amount(200)}}, const std::map<MosaicId, Amount>& unlockMosaics = {{MosaicId(72), Amount(200)}})
		{
			return  [&](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta) {
				accountStateCacheDelta.addAccount(signer, Height(1), 2);
				auto& account = accountStateCacheDelta.find(signer).get();
				for(auto mosaic : mosaics)
				{
					account.Balances.credit(mosaic.first, mosaic.second);
					account.Balances.lock(mosaic.first, mosaic.second);
				}
				lockFundCacheDelta.insert(signer, unlockHeight, unlockMosaics);

			};
		}
		template<typename TSeedCacheFunc, typename TCheckCacheFunc>
		void RunTest(
				NotifyMode notifyMode,
				TSeedCacheFunc seedCache,
				TCheckCacheFunc checkCache) {
			// Arrange:
			// Prepare Configurations
			auto initialConfiguration = test::MutableBlockchainConfiguration();
			auto modifiedConfiguration = test::MutableBlockchainConfiguration();

			auto pluginConfig = config::LockFundConfiguration::Uninitialized();
			pluginConfig.Enabled = false;

			auto pluginConfigNew = config::LockFundConfiguration::Uninitialized();
			pluginConfigNew.Enabled = true;

			initialConfiguration.Network.SetPluginConfiguration(pluginConfig);
			modifiedConfiguration.Network.SetPluginConfiguration(pluginConfigNew);

			auto oldConfiguration = initialConfiguration.ToConst();
			modifiedConfiguration.PreviousConfiguration = &oldConfiguration;
			modifiedConfiguration.ActivationHeight = Height(777);
			modifiedConfiguration.Network.MaxRollbackBlocks = 10;
			modifiedConfiguration.Immutable.HarvestingMosaicId = MosaicId(72);

			ObserverTestContext context(notifyMode, Height(777), modifiedConfiguration.ToConst());
			auto& globalStore = context.cache().template sub<cache::GlobalStoreCache>();
			auto& lockFundCacheDelta = context.cache().sub<cache::LockFundCache>();
			auto& accountStateCacheDelta = context.cache().sub<cache::AccountStateCache>();
			seedCache(lockFundCacheDelta, accountStateCacheDelta);

			state::GlobalEntry stakingRecord(config::TotalStaked_GlobalKey, 1000, state::Uint64Converter());
			globalStore.insert(stakingRecord);

			state::GlobalEntry installed(config::LockFundPluginInstalled_GlobalKey, 1000, state::Uint64Converter());
			globalStore.insert(installed);


			auto notification = test::CreateBlockNotification();
			auto pObserver = CreateLockFundBlockObserver();

			// Act:
			test::ObserveNotification(*pObserver, notification, context);



			// Assert:
			auto& stakingCacheRecord = globalStore.find(config::TotalStaked_GlobalKey).get();
			checkCache(lockFundCacheDelta, accountStateCacheDelta, stakingCacheRecord.Get<state::Uint64Converter>());

			if(notifyMode != NotifyMode::Rollback)
			{
				auto pStatement = context.statementBuilder().build();
				ASSERT_EQ(1u, pStatement->BlockchainStateStatements.size());
				const auto& receiptPair = *pStatement->BlockchainStateStatements.find(model::ReceiptSource());
				ASSERT_EQ(1u, receiptPair.second.size());

				const auto& receipt = static_cast<const model::TotalStakedReceipt&>(receiptPair.second.receiptAt(0));
				EXPECT_EQ(receipt.Amount, Amount(stakingCacheRecord.Get<state::Uint64Converter>()));
			}
		}
	}

	// region testing

	TEST(TEST_CLASS, ObserverCanUnlockFunds_CommitUnlocksAndInactivates) {
		// Arrange
		auto signer = test::GenerateRandomByteArray<Key>();
		// Act: add it
		RunTest(
			NotifyMode::Commit,
			PrepareDefaultAccount(signer, Height(777)),
			[&signer](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta, uint64_t totalStaked) {
			  // Assert: balances was locked but no unlocking record was added.Some balance still persists in the regular balance and the remainder is now locked
			  auto account = accountStateCacheDelta.find(signer).get();
			  auto lockFundKeyRecord = lockFundCacheDelta.find(signer).tryGet();
			  EXPECT_TRUE(lockFundKeyRecord);
			  EXPECT_EQ(1u, account.Balances.size());
			  EXPECT_EQ(0u, account.Balances.lockedBalances().size());
			  EXPECT_EQ(1u, account.Balances.balances().size());
			  EXPECT_EQ(200, account.Balances.get(MosaicId(72)).unwrap());
			  EXPECT_EQ(0, account.Balances.getLocked(MosaicId(72)).unwrap());
			  EXPECT_EQ(totalStaked, 800);
			  EXPECT_EQ(lockFundKeyRecord->LockFundRecords.size(), 1);
			  EXPECT_FALSE(lockFundKeyRecord->LockFundRecords.at(Height(777)).Active());
		});
	}

	TEST(TEST_CLASS, ObserverCanUnlockFunds_CommitUnlocksAndRemovesOnceInactive) {
		// Arrange
		auto signer = test::GenerateRandomByteArray<Key>();
		// Act: add it
		RunTest(
				NotifyMode::Commit,
				[&](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta){
			  		PrepareDefaultAccount(signer, Height(777-10))(lockFundCacheDelta, accountStateCacheDelta);
					lockFundCacheDelta.disable(signer, Height(777-10));
				},
				[&signer](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta, uint64_t totalStaked) {
				  // Assert: balances was locked but no unlocking record was added.Some balance still persists in the regular balance and the remainder is now locked
				  auto account = accountStateCacheDelta.find(signer).get();
				  auto lockFundKeyRecord = lockFundCacheDelta.find(signer).tryGet();
				  EXPECT_FALSE(lockFundKeyRecord);
				  EXPECT_EQ(1u, account.Balances.size());
				  EXPECT_EQ(1u, account.Balances.lockedBalances().size());
				  EXPECT_EQ(0u, account.Balances.balances().size());
				  EXPECT_EQ(0, account.Balances.get(MosaicId(72)).unwrap());
				  EXPECT_EQ(200, account.Balances.getLocked(MosaicId(72)).unwrap());
				  EXPECT_EQ(totalStaked, 1000);
				});
	}

	TEST(TEST_CLASS, ObserverCanUnlockFunds_CommitRollsback) {
		// Arrange
		auto signer = test::GenerateRandomByteArray<Key>();
		// Act: add it
		RunTest(
				NotifyMode::Rollback,
				[&](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta){
				  accountStateCacheDelta.addAccount(signer, Height(1), 2);
				  auto& account = accountStateCacheDelta.find(signer).get();
				  account.Balances.credit(MosaicId(72), Amount(200));
				  lockFundCacheDelta.insert(signer, Height(777), {{MosaicId(72), Amount(200)}});
				  lockFundCacheDelta.disable(signer, Height(777));
				},
				[&signer](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta, uint64_t totalStaked) {
				  // Assert: balances was locked but no unlocking record was added.Some balance still persists in the regular balance and the remainder is now locked
				  auto account = accountStateCacheDelta.find(signer).get();
				  auto lockFundKeyRecord = lockFundCacheDelta.find(signer).tryGet();
				  EXPECT_TRUE(lockFundKeyRecord);
				  EXPECT_EQ(1u, account.Balances.size());
				  EXPECT_EQ(1u, account.Balances.lockedBalances().size());
				  EXPECT_EQ(0u, account.Balances.balances().size());
				  EXPECT_EQ(0, account.Balances.get(MosaicId(72)).unwrap());
				  EXPECT_EQ(200, account.Balances.getLocked(MosaicId(72)).unwrap());
				  EXPECT_EQ(totalStaked, 1200);
				  EXPECT_EQ(lockFundKeyRecord->LockFundRecords.size(), 1);
				  EXPECT_TRUE(lockFundKeyRecord->LockFundRecords.at(Height(777)).Active());
				});
	}

	// endregion

	// install region

	void RunTest(
			bool isInstalled,
			NotifyMode notifyMode) {
		// Arrange:
		// Prepare Configurations
		auto initialConfiguration = test::MutableBlockchainConfiguration();
		auto modifiedConfiguration = test::MutableBlockchainConfiguration();

		auto pluginConfig = config::LockFundConfiguration::Uninitialized();
		pluginConfig.Enabled = false;

		auto pluginConfigNew = config::LockFundConfiguration::Uninitialized();
		pluginConfigNew.Enabled = true;

		initialConfiguration.Network.SetPluginConfiguration(pluginConfig);
		modifiedConfiguration.Network.SetPluginConfiguration(pluginConfigNew);

		auto oldConfiguration = initialConfiguration.ToConst();
		modifiedConfiguration.PreviousConfiguration = &oldConfiguration;
		modifiedConfiguration.ActivationHeight = Height(777);

		ObserverTestContext context(notifyMode, Height(777), modifiedConfiguration.ToConst());
		auto& globalStore = context.cache().template sub<cache::GlobalStoreCache>();
		if(isInstalled)
		{
			state::GlobalEntry stakingRecord(config::TotalStaked_GlobalKey, 1000, state::Uint64Converter());
			globalStore.insert(stakingRecord);
			state::GlobalEntry installedRecord(config::LockFundPluginInstalled_GlobalKey, 5, state::PluginInstallConverter());
			globalStore.insert(installedRecord);
		}
		else if(notifyMode == NotifyMode::Rollback)
		{
			state::GlobalEntry installedRecord(config::LockFundPluginInstalled_GlobalKey, 777, state::PluginInstallConverter());
			globalStore.insert(installedRecord);
		}


		auto notification = test::CreateBlockNotification();
		auto pObserver = CreateLockFundBlockObserver();

		// Act:
		test::ObserveNotification(*pObserver, notification, context);



		// Assert:
		if(!isInstalled)
		{
			if(notifyMode == NotifyMode::Commit)
			{
				auto installationKey = globalStore.find(config::LockFundPluginInstalled_GlobalKey).get();
				auto value = installationKey.template Get<state::PluginInstallConverter>();
				EXPECT_EQ(value, 777);
			}

			else
			{
				EXPECT_FALSE(globalStore.find(config::LockFundPluginInstalled_GlobalKey).tryGet());
			}
		}
		else
		{
			auto installationKey = globalStore.find(config::LockFundPluginInstalled_GlobalKey).get();
			auto value = installationKey.template Get<state::PluginInstallConverter>();
			EXPECT_EQ(value, 5);
		}

	}

	TEST(TEST_CLASS, ObserverCanInstallPlugin_NotInstalled_Commit) {
		// Arrange
		RunTest(false, NotifyMode::Commit);
	}

	TEST(TEST_CLASS, ObserverCanInstallPlugin_NotInstalled_Rollback) {
		// Arrange
		RunTest(false, NotifyMode::Commit);
	}

	TEST(TEST_CLASS, ObserverCanInstallPlugin_Installed_Commit) {
		// Arrange
		RunTest(true, NotifyMode::Rollback);
	}

	TEST(TEST_CLASS, ObserverCanInstallPlugin_Installed_Rollback) {
		// Arrange
		RunTest(true, NotifyMode::Rollback);
	}
	// endregion
}}
