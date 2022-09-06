/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "plugins/services/globalstore/src/cache/GlobalStoreCache.h"
#include "plugins/services/globalstore/src/state/BaseConverters.h"

#include "tests/test/LockFundCacheFactory.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "src/cache/LockFundCache.h"
#include "catapult/model/ResolverContext.h"
#include "src/config/LockFundConfiguration.h"
#include "src/observers/Observers.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS LockFundTransferObserverTests

	using ObserverTestContext = test::ObserverTestContextT<test::LockFundCacheFactory>;

	DEFINE_COMMON_OBSERVER_TESTS(LockFundTransfer,)

	namespace {

		model::LockFundTransferNotification<1> CreateTransferNotification(const Key& signer, const std::vector<model::UnresolvedMosaic>& mosaics, BlockDuration blocksUntilUnlock, model::LockFundAction lockFundAction) {
			return model::LockFundTransferNotification<1>(signer, blocksUntilUnlock, mosaics, lockFundAction);
		}

		template<typename TSeedCacheFunc, typename TCheckCacheFunc>
		void RunChildTest(
				const model::LockFundTransferNotification<1>& notification,
				ObserverTestContext& context,
				TSeedCacheFunc seedCache,
				TCheckCacheFunc checkCache) {
			// Arrange:
			auto pObserver = CreateLockFundTransferObserver();

			// - seed the cache
			auto& globalStore = context.cache().sub<cache::GlobalStoreCache>();
			if(context.observerContext().Mode == NotifyMode::Commit)
			{
				state::GlobalEntry stakingRecord(config::TotalStaked_GlobalKey, 1000, state::Uint64Converter());
				globalStore.insert(stakingRecord);
			}
			auto& lockFundCacheDelta = context.cache().sub<cache::LockFundCache>();
			auto& accountStateCacheDelta = context.cache().sub<cache::AccountStateCache>();
			seedCache(lockFundCacheDelta, accountStateCacheDelta);

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			auto& stakingCacheRecord = globalStore.find(config::TotalStaked_GlobalKey).get();
			checkCache(lockFundCacheDelta, accountStateCacheDelta, stakingCacheRecord.Get<state::Uint64Converter>());
		}

		auto PrepareDefaultAccount(const Key& signer, std::map<MosaicId, Amount> mosaics = {{MosaicId(72), Amount(200)}})
		{
			return  [&signer, &mosaics](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta) {
				accountStateCacheDelta.addAccount(signer, Height(1), 2);
				auto& account = accountStateCacheDelta.find(signer).get();
				for(auto mosaic : mosaics)
					account.Balances.credit(mosaic.first, mosaic.second);

			};
		}

		auto PrepareConfiguration(uint8_t maxMosaicsSize, BlockDuration unlockCooldown)
		{
			auto pluginConfig = config::LockFundConfiguration::Uninitialized();
			pluginConfig.MaxMosaicsSize = maxMosaicsSize;
			pluginConfig.MaxUnlockRequests = 100;
			pluginConfig.MinRequestUnlockCooldown = unlockCooldown;
			test::MutableBlockchainConfiguration mutableConfig;
			mutableConfig.Immutable.HarvestingMosaicId = MosaicId(72);
			mutableConfig.Network.SetPluginConfiguration(pluginConfig);
			return mutableConfig.ToConst();
		}

		auto PrepareContext(Height height, uint8_t maxMosaicsSize, BlockDuration unlockCooldown, NotifyMode mode = NotifyMode::Commit)
		{
			return ObserverTestContext(mode, height, PrepareConfiguration(maxMosaicsSize, unlockCooldown), model::ResolverContext());
		}

		auto VerifyDefaultCacheStateAfterRollback(const model::LockFundTransferNotification<1>& notification, const Key& signer, ObserverTestContext& context, std::map<MosaicId, Amount> mosaics = {{MosaicId(72), Amount(200)}})
		{
			RunChildTest(
			notification,
			context,
			[](auto&, auto&){},
			[&signer, &mosaics](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta, uint64_t totalStaked) {
			  // Assert: balances was locked but no unlocking record was added.Some balance still persists in the regular balance and the remainder is now locked
			  auto account = accountStateCacheDelta.find(signer).get();
			  auto lockFundKeyRecord = lockFundCacheDelta.find(signer).tryGet();
			  EXPECT_FALSE(lockFundKeyRecord);
			  auto lockFundHeightRecord = lockFundCacheDelta.find(Height(10)).tryGet();
			  EXPECT_FALSE(lockFundHeightRecord);
			  EXPECT_EQ(mosaics.size(), account.Balances.size());
			  EXPECT_EQ(0u, account.Balances.lockedBalances().size());
			  EXPECT_EQ(mosaics.size(), account.Balances.balances().size());
			  for(auto& mosaic : mosaics)
			  {
				  EXPECT_EQ(mosaic.second, account.Balances.get(mosaic.first));
				  EXPECT_EQ(0, account.Balances.getLocked(mosaic.first).unwrap());
			  }
			  EXPECT_EQ(totalStaked, 1000);
			});
		}
	}

	// region locking

	TEST(TEST_CLASS, ObserverLocksMosaicOnCommit) {
		// Arrange
		auto signer = test::GenerateRandomByteArray<Key>();
		std::vector<model::UnresolvedMosaic> mosaics = { { UnresolvedMosaicId(72), UnresolvedAmount(100) } };
		auto notification = CreateTransferNotification(signer, mosaics, BlockDuration(0), model::LockFundAction::Lock);
		auto context = PrepareContext(Height{444}, 256, BlockDuration(10));
		// Act: add it
		RunChildTest(
				notification,
				context,
				PrepareDefaultAccount(signer),
				[&signer](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta, uint64_t totalStaked) {
				  // Assert: balances was locked but no unlocking record was added.Some balance still persists in the regular balance and the remainder is now locked
				  auto account = accountStateCacheDelta.find(signer).get();
				  auto lockFundKeyRecord = lockFundCacheDelta.find(signer).tryGet();
				  EXPECT_FALSE(lockFundKeyRecord);
				  EXPECT_EQ(1u, account.Balances.size());
				  EXPECT_EQ(1u, account.Balances.lockedBalances().size());
				  EXPECT_EQ(1u, account.Balances.balances().size());
				  EXPECT_EQ(100, account.Balances.get(MosaicId(72)).unwrap());
				  EXPECT_EQ(100, account.Balances.getLocked(MosaicId(72)).unwrap());
				  EXPECT_EQ(totalStaked, 1100);
				});

		auto newContext = context.alterMode(NotifyMode::Rollback);
		// Act: rollback and verify
		VerifyDefaultCacheStateAfterRollback(notification, signer, newContext);
	}

	TEST(TEST_CLASS, ObserverLocksMultipleMosaicsOnCommit) {
		// Arrange
		auto signer = test::GenerateRandomByteArray<Key>();
		std::vector<model::UnresolvedMosaic> mosaics = {
				{ UnresolvedMosaicId(72), UnresolvedAmount(100) },
				{ UnresolvedMosaicId(172), UnresolvedAmount(100) },
				{ UnresolvedMosaicId(200), UnresolvedAmount(100) }
		};
		auto notification = CreateTransferNotification(signer, mosaics,
																		BlockDuration(0), model::LockFundAction::Lock);
		auto context = PrepareContext(Height{444}, 256, BlockDuration(10));
		std::map<MosaicId, Amount> initialMosaics = { { MosaicId(72), Amount(200) }, { MosaicId(172), Amount(100) }, { MosaicId(200), Amount(200) } };

		// Act: add it
		RunChildTest(
				notification,
				context,
				PrepareDefaultAccount(signer, initialMosaics),
				[&signer](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta, uint64_t totalStaked) {
				  // Assert: balances was locked but no unlocking record was added.Some balance still persists in the regular balance and the remainder is now locked
				  auto account = accountStateCacheDelta.find(signer).get();
				  auto lockFundKeyRecord = lockFundCacheDelta.find(signer).tryGet();
				  EXPECT_FALSE(lockFundKeyRecord);
				  EXPECT_EQ(3u, account.Balances.size());
				  EXPECT_EQ(3u, account.Balances.lockedBalances().size());
				  EXPECT_EQ(2u, account.Balances.balances().size());
				  EXPECT_EQ(100, account.Balances.get(MosaicId(72)).unwrap());
				  EXPECT_EQ(100, account.Balances.getLocked(MosaicId(72)).unwrap());
				  EXPECT_EQ(0, account.Balances.get(MosaicId(172)).unwrap());
				  EXPECT_EQ(100, account.Balances.getLocked(MosaicId(172)).unwrap());
				  EXPECT_EQ(100, account.Balances.get(MosaicId(200)).unwrap());
				  EXPECT_EQ(100, account.Balances.getLocked(MosaicId(200)).unwrap());
				  EXPECT_EQ(totalStaked, 1100);
				});

		auto newContext = context.alterMode(NotifyMode::Rollback);
		// Act: rollback and verify
		VerifyDefaultCacheStateAfterRollback(notification, signer, newContext, initialMosaics);
	}

	TEST(TEST_CLASS, ObserverLocksAllOfMosaicOnCommit) {
		// Arrange
		auto signer = test::GenerateRandomByteArray<Key>();
		std::vector<model::UnresolvedMosaic> mosaics = { { UnresolvedMosaicId(72), UnresolvedAmount(200) } };
		auto notification = CreateTransferNotification(signer, mosaics, BlockDuration(0), model::LockFundAction::Lock);
		auto context = PrepareContext(Height{444}, 256, BlockDuration(10));
		// Act: add it
		RunChildTest(
				notification,
				context,
				PrepareDefaultAccount(signer),
				[&signer](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta, uint64_t totalStaked) {
				  // Assert: balances was locked but no unlocking record was added.All funds are no locked so regular balance is empty.
				  auto account = accountStateCacheDelta.find(signer).get();
				  auto lockFundKeyRecord = lockFundCacheDelta.find(signer).tryGet();
				  EXPECT_FALSE(lockFundKeyRecord);
				  EXPECT_EQ(1u, account.Balances.size());
				  EXPECT_EQ(1u, account.Balances.lockedBalances().size());
				  EXPECT_EQ(0u, account.Balances.balances().size());
				  EXPECT_EQ(0, account.Balances.get(MosaicId(72)).unwrap());
				  EXPECT_EQ(200, account.Balances.getLocked(MosaicId(72)).unwrap());
				  EXPECT_EQ(totalStaked, 1200);
				});

		auto newContext = context.alterMode(NotifyMode::Rollback);
		// Act: rollback and verify
		VerifyDefaultCacheStateAfterRollback(notification, signer, newContext);
	}

	TEST(TEST_CLASS, ObserverLocksAllOfMultipleMosaicsOnCommit) {
		// Arrange
		auto signer = test::GenerateRandomByteArray<Key>();
		std::vector<model::UnresolvedMosaic> mosaics = {
				{ UnresolvedMosaicId(72), UnresolvedAmount(200) },
				{ UnresolvedMosaicId(172), UnresolvedAmount(100) },
				{ UnresolvedMosaicId(200), UnresolvedAmount(200) }
		};
		auto notification = CreateTransferNotification(signer, mosaics,
													   BlockDuration(0), model::LockFundAction::Lock);
		auto context = PrepareContext(Height{444}, 256, BlockDuration(10));
		std::map<MosaicId, Amount> initialMosaics = { { MosaicId(72), Amount(200) }, { MosaicId(172), Amount(200) }, { MosaicId(200), Amount(200) } };
		// Act: add it
		RunChildTest(
				notification,
				context,
				PrepareDefaultAccount(signer, initialMosaics),
				[&signer](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta, uint64_t totalStaked) {
				  // Assert: balances was locked but no unlocking record was added.Some balance still persists in the regular balance and the remainder is now locked
				  auto account = accountStateCacheDelta.find(signer).get();
				  auto lockFundKeyRecord = lockFundCacheDelta.find(signer).tryGet();
				  EXPECT_FALSE(lockFundKeyRecord);
				  EXPECT_EQ(3u, account.Balances.size());
				  EXPECT_EQ(3u, account.Balances.lockedBalances().size());
				  EXPECT_EQ(1u, account.Balances.balances().size());
				  EXPECT_EQ(0, account.Balances.get(MosaicId(72)).unwrap());
				  EXPECT_EQ(200, account.Balances.getLocked(MosaicId(72)).unwrap());
				  EXPECT_EQ(100, account.Balances.get(MosaicId(172)).unwrap());
				  EXPECT_EQ(100, account.Balances.getLocked(MosaicId(172)).unwrap());
				  EXPECT_EQ(0, account.Balances.get(MosaicId(200)).unwrap());
				  EXPECT_EQ(200, account.Balances.getLocked(MosaicId(200)).unwrap());
				  EXPECT_EQ(totalStaked, 1200);
				});

		auto newContext = context.alterMode(NotifyMode::Rollback);
		// Act: rollback and verify
		VerifyDefaultCacheStateAfterRollback(notification, signer, newContext, initialMosaics);
	}

	TEST(TEST_CLASS, ObserverLocksAllOfMosaicOnCommitAndGeneratesUnlockRecord) {
		// Arrange: create a child namespace with a root parent
		auto signer = test::GenerateRandomByteArray<Key>();
		std::vector<model::UnresolvedMosaic> mosaics = { { UnresolvedMosaicId(72), UnresolvedAmount(200) } };
		auto notification = CreateTransferNotification(signer, mosaics, BlockDuration(10), model::LockFundAction::Lock);
		auto context = PrepareContext(Height{10}, 256, BlockDuration(10));

		// Act: add it
		RunChildTest(
		notification,
		context,
		PrepareDefaultAccount(signer),
		[&signer](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta, uint64_t totalStaked) {
		  // Assert: balances was locked but no unlocking record was added.All funds are no locked so regular balance is empty.
		  auto account = accountStateCacheDelta.find(signer).get();
		  auto lockFundKeyRecord = lockFundCacheDelta.find(signer).tryGet();
		  auto lockFundHeightRecord = lockFundCacheDelta.find(Height(20)).tryGet();

		  //Validate unlocking records
		  EXPECT_TRUE(lockFundKeyRecord);
		  EXPECT_TRUE(lockFundHeightRecord);
		  EXPECT_EQ(lockFundKeyRecord->LockFundRecords.size(), lockFundHeightRecord->LockFundRecords.size());
		  EXPECT_EQ(lockFundKeyRecord->LockFundRecords.size(), 1);
		  EXPECT_TRUE(lockFundHeightRecord->LockFundRecords.find(signer)->second.Active());
		  EXPECT_FALSE(lockFundHeightRecord->LockFundRecords.find(signer)->second.InactiveRecords.size());
		  EXPECT_TRUE(lockFundKeyRecord->LockFundRecords.find(Height(20))->second.Active());
		  EXPECT_FALSE(lockFundKeyRecord->LockFundRecords.find(Height(20))->second.InactiveRecords.size());
		  EXPECT_EQ(lockFundKeyRecord->Identifier, signer);
		  EXPECT_EQ(lockFundHeightRecord->Identifier, Height(20));

		  //Validate unlocking amounts

		  auto keyRecord = lockFundKeyRecord->LockFundRecords.find(Height(20))->second.Get();
		  auto heightRecord = lockFundHeightRecord->LockFundRecords.find(signer)->second.Get();
		  EXPECT_EQ(heightRecord.size(), 1);
		  EXPECT_EQ(heightRecord.find(MosaicId(72))->second.unwrap(), 200);
		  EXPECT_EQ(keyRecord.size(), 1);
		  EXPECT_EQ(keyRecord.find(MosaicId(72))->second.unwrap(), 200);

		  //Validate Balances
		  EXPECT_EQ(1u, account.Balances.size());
		  EXPECT_EQ(1u, account.Balances.lockedBalances().size());
		  EXPECT_EQ(0u, account.Balances.balances().size());
		  EXPECT_EQ(0, account.Balances.get(MosaicId(72)).unwrap());
		  EXPECT_EQ(200, account.Balances.getLocked(MosaicId(72)).unwrap());
		  EXPECT_EQ(totalStaked, 1200);
		});
		// Act: rollback and verify
		auto newContext = context.alterMode(NotifyMode::Rollback);
		VerifyDefaultCacheStateAfterRollback(notification, signer, newContext);
	}

	TEST(TEST_CLASS, ObserverLocksAllOfMultipleMosaicsOnCommitAndGeneratesUnlockRecord) {
		// Arrange: create a child namespace with a root parent
		auto signer = test::GenerateRandomByteArray<Key>();
		std::vector<model::UnresolvedMosaic> mosaics = {
				{ UnresolvedMosaicId(72), UnresolvedAmount(200) },
				{ UnresolvedMosaicId(172), UnresolvedAmount(100) },
				{ UnresolvedMosaicId(200), UnresolvedAmount(200) }
		};
		auto notification = CreateTransferNotification(signer, mosaics, BlockDuration(10), model::LockFundAction::Lock);
		auto context = PrepareContext(Height{10}, 256, BlockDuration(10));
		std::map<MosaicId, Amount> initialMosaics = { { MosaicId(72), Amount(200) }, { MosaicId(172), Amount(100) }, { MosaicId(200), Amount(200) } };

		// Act: add it
		RunChildTest(
				notification,
				context,
				PrepareDefaultAccount(signer, initialMosaics),
				[&signer, &initialMosaics](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta, uint64_t totalStaked) {
				  // Assert: balances was locked but no unlocking record was added.All funds are no locked so regular balance is empty.
				  auto account = accountStateCacheDelta.find(signer).get();
				  auto lockFundKeyRecord = lockFundCacheDelta.find(signer).tryGet();
				  auto lockFundHeightRecord = lockFundCacheDelta.find(Height(20)).tryGet();

				  //Validate unlocking records
				  EXPECT_TRUE(lockFundKeyRecord);
				  EXPECT_TRUE(lockFundHeightRecord);
				  EXPECT_EQ(lockFundKeyRecord->LockFundRecords.size(), lockFundHeightRecord->LockFundRecords.size());
				  EXPECT_EQ(lockFundKeyRecord->LockFundRecords.size(), 1);
				  EXPECT_TRUE(lockFundHeightRecord->LockFundRecords.find(signer)->second.Active());
				  EXPECT_FALSE(lockFundHeightRecord->LockFundRecords.find(signer)->second.InactiveRecords.size());
				  EXPECT_TRUE(lockFundKeyRecord->LockFundRecords.find(Height(20))->second.Active());
				  EXPECT_FALSE(lockFundKeyRecord->LockFundRecords.find(Height(20))->second.InactiveRecords.size());
				  EXPECT_EQ(lockFundKeyRecord->Identifier, signer);
				  EXPECT_EQ(lockFundHeightRecord->Identifier, Height(20));

				  //Validate unlocking amounts
				  auto keyRecord = lockFundKeyRecord->LockFundRecords.find(Height(20))->second.Get();
				  auto heightRecord = lockFundHeightRecord->LockFundRecords.find(signer)->second.Get();
				  EXPECT_EQ(heightRecord.size(), initialMosaics.size());
				  EXPECT_EQ(keyRecord.size(), initialMosaics.size());
				  for(auto mosaic : initialMosaics)
				  {
					  EXPECT_EQ(heightRecord.find(mosaic.first)->second, mosaic.second);
					  EXPECT_EQ(keyRecord.find(mosaic.first)->second, mosaic.second);
				  }

				  //Validate Balances

				  EXPECT_EQ(initialMosaics.size(), account.Balances.size());
				  EXPECT_EQ(initialMosaics.size(), account.Balances.lockedBalances().size());
				  EXPECT_EQ(0u, account.Balances.balances().size());

				  for(auto mosaic : initialMosaics)
				  {
					  EXPECT_EQ(0, account.Balances.get(mosaic.first).unwrap());
					  EXPECT_EQ(mosaic.second, account.Balances.getLocked(mosaic.first));
				  }
				  EXPECT_EQ(totalStaked, 1200);

				});
		// Act: rollback and verify
		auto newContext = context.alterMode(NotifyMode::Rollback);
		VerifyDefaultCacheStateAfterRollback(notification, signer, newContext, initialMosaics);
	}

	// endregion

	// region unlocking
	TEST(TEST_CLASS, ObserverUnlockingMultipleMosaicsWithZeroDurationGeneratesUnlockRecordWithMinimumDuration) {
		// Arrange: create a child namespace with a root parent
		auto signer = test::GenerateRandomByteArray<Key>();
		std::vector<model::UnresolvedMosaic> mosaics = {
				{ UnresolvedMosaicId(72), UnresolvedAmount(200) },
				{ UnresolvedMosaicId(172), UnresolvedAmount(100) },
				{ UnresolvedMosaicId(200), UnresolvedAmount(200) }
		};
		auto notification = CreateTransferNotification(signer, mosaics, BlockDuration(0), model::LockFundAction::Unlock);
		auto context = PrepareContext(Height{10}, 256, BlockDuration(10));
		std::map<MosaicId, Amount> initialMosaics = { { MosaicId(72), Amount(200) }, { MosaicId(172), Amount(100) }, { MosaicId(200), Amount(200) } };

		// Act: add it
		RunChildTest(
				notification,
				context,
				PrepareDefaultAccount(signer, initialMosaics), //To be able to use the test rollback function
				[&signer, &initialMosaics](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta, uint64_t totalStaked) {
				  // Assert: balances was locked but no unlocking record was added.All funds are no locked so regular balance is empty.
				  auto account = accountStateCacheDelta.find(signer).get();
				  auto lockFundKeyRecord = lockFundCacheDelta.find(signer).tryGet();
				  auto lockFundHeightRecord = lockFundCacheDelta.find(Height(20)).tryGet();

				  //Validate unlocking records
				  EXPECT_TRUE(lockFundKeyRecord);
				  EXPECT_TRUE(lockFundHeightRecord);
				  EXPECT_EQ(lockFundKeyRecord->LockFundRecords.size(), lockFundHeightRecord->LockFundRecords.size());
				  EXPECT_EQ(lockFundKeyRecord->LockFundRecords.size(), 1);
				  EXPECT_TRUE(lockFundHeightRecord->LockFundRecords.find(signer)->second.Active());
				  EXPECT_FALSE(lockFundHeightRecord->LockFundRecords.find(signer)->second.InactiveRecords.size());
				  EXPECT_TRUE(lockFundKeyRecord->LockFundRecords.find(Height(20))->second.Active());
				  EXPECT_FALSE(lockFundKeyRecord->LockFundRecords.find(Height(20))->second.InactiveRecords.size());
				  EXPECT_EQ(lockFundKeyRecord->Identifier, signer);
				  EXPECT_EQ(lockFundHeightRecord->Identifier, Height(20));

				  //Validate unlocking amounts
				  auto keyRecord = lockFundKeyRecord->LockFundRecords.find(Height(20))->second.Get();
				  auto heightRecord = lockFundHeightRecord->LockFundRecords.find(signer)->second.Get();
				  EXPECT_EQ(heightRecord.size(), initialMosaics.size());
				  EXPECT_EQ(keyRecord.size(), initialMosaics.size());
				  for(auto mosaic : initialMosaics)
				  {
					  EXPECT_EQ(heightRecord.find(mosaic.first)->second, mosaic.second);
					  EXPECT_EQ(keyRecord.find(mosaic.first)->second, mosaic.second);
				  }

				});
		// Act: rollback and verify
		auto newContext = context.alterMode(NotifyMode::Rollback);
		VerifyDefaultCacheStateAfterRollback(notification, signer, newContext, initialMosaics);
	}
	// endregion
}}
