/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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
#include "src/cache/LockFundCache.h"
#include "src/config/LockFundConfiguration.h"
#include "src/observers/Observers.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS LockFundTransferObserverTests

	using ObserverTestContext = test::ObserverTestContextT<test::CoreSystemCacheFactory>;

	DEFINE_COMMON_OBSERVER_TESTS(LockFundTransfer,)

	namespace {

		model::LockFundTransferNotification<1> CreateTransferNotification(const Key& signer, const std::vector<model::UnresolvedMosaic>& mosaics, BlockDuration blocksUntilUnlock, model::LockFundAction lockFundAction) {
			return model::LockFundTransferNotification<1>(signer, mosaics.size(), blocksUntilUnlock, mosaics.data(), lockFundAction);
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
			auto& lockFundCacheDelta = context.cache().sub<cache::LockFundCache>();
			auto& accountStateCacheDelta = context.cache().sub<cache::AccountStateCache>();
			seedCache(lockFundCacheDelta, accountStateCacheDelta);
			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			checkCache(lockFundCacheDelta, accountStateCacheDelta);
		}

		auto PrepareDefaultAccount(const Key& signer, std::map<MosaicId, Amount> mosaics = {{MosaicId(72), Amount(200)}})
		{
			return [&signer, &mosaics](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta) {
				accountStateCacheDelta.addAccount(signer, Height(1), 2);
				auto account = accountStateCacheDelta.find(signer).get();
				for(auto mosaic : mosaics)
					account.Balances.credit(mosaic.first, mosaic.second);

			};
		}

		auto PrepareConfiguration(uint8_t maxMosaicsSize, BlockDuration unlockCooldown)
		{
			auto pluginConfig = config::LockFundConfiguration::Uninitialized();
			pluginConfig.MaxMosaicsSize = maxMosaicsSize;
			pluginConfig.MinRequestUnlockCooldown = unlockCooldown;
			test::MutableBlockchainConfiguration mutableConfig;
			mutableConfig.Network.SetPluginConfiguration(pluginConfig);
			return mutableConfig.ToConst();
		}

		auto PrepareContext(Height height, uint8_t maxMosaicsSize, BlockDuration unlockCooldown, NotifyMode mode = NotifyMode::Commit)
		{
			return ObserverTestContext(mode, height, PrepareConfiguration(maxMosaicsSize, unlockCooldown));
		}

		auto VerifyDefaultCacheStateAfterRollback(const model::LockFundTransferNotification<1>& notification, const Key& signer, ObserverTestContext& context, std::map<MosaicId, Amount> mosaics = {{MosaicId(72), Amount(200)}})
		{
			RunChildTest(
			notification,
			context,
			[](auto&, auto&){},
			[&signer, &mosaics](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta) {
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
				  EXPECT_EQ(0, account.Balances.getLocked(mosaic.first));
			  }
			});
		}
	}

	// region locking

	TEST(TEST_CLASS, ObserverLocksMosaicOnCommit) {
		// Arrange
		auto signer = test::GenerateRandomByteArray<Key>();
		auto notification = CreateTransferNotification(signer, { { UnresolvedMosaicId(72), UnresolvedAmount(100) } }, BlockDuration(0), model::LockFundAction::Lock);
		auto context = PrepareContext(Height{444}, 256, BlockDuration(10));
		// Act: add it
		RunChildTest(
				notification,
				context,
				PrepareDefaultAccount(signer),
				[&signer](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta) {
				  // Assert: balances was locked but no unlocking record was added.Some balance still persists in the regular balance and the remainder is now locked
				  auto account = accountStateCacheDelta.find(signer).get();
				  auto lockFundKeyRecord = lockFundCacheDelta.find(signer).tryGet();
				  EXPECT_FALSE(lockFundKeyRecord);
				  EXPECT_EQ(1u, account.Balances.size());
				  EXPECT_EQ(1u, account.Balances.lockedBalances().size());
				  EXPECT_EQ(1u, account.Balances.balances().size());
				  EXPECT_EQ(100, account.Balances.get(MosaicId(72)));
				  EXPECT_EQ(100, account.Balances.getLocked(MosaicId(72)));
				});

		auto newContext = context.alterMode(NotifyMode::Rollback);
		// Act: rollback and verify
		VerifyDefaultCacheStateAfterRollback(notification, signer, newContext);
	}

	TEST(TEST_CLASS, ObserverLocksMultipleMosaicsOnCommit) {
		// Arrange
		auto signer = test::GenerateRandomByteArray<Key>();
		auto notification = CreateTransferNotification(signer, {
																		{ UnresolvedMosaicId(72), UnresolvedAmount(100) },
																		{ UnresolvedMosaicId(172), UnresolvedAmount(100) },
																		{ UnresolvedMosaicId(200), UnresolvedAmount(100) }
															   		   },
																		BlockDuration(0), model::LockFundAction::Lock);
		auto context = PrepareContext(Height{444}, 256, BlockDuration(10));
		std::map<MosaicId, Amount> mosaics = { { MosaicId(72), Amount(200) }, { MosaicId(172), Amount(100) }, { MosaicId(200), Amount(200) } };

		// Act: add it
		RunChildTest(
				notification,
				context,
				PrepareDefaultAccount(signer, mosaics),
				[&signer](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta) {
				  // Assert: balances was locked but no unlocking record was added.Some balance still persists in the regular balance and the remainder is now locked
				  auto account = accountStateCacheDelta.find(signer).get();
				  auto lockFundKeyRecord = lockFundCacheDelta.find(signer).tryGet();
				  EXPECT_FALSE(lockFundKeyRecord);
				  EXPECT_EQ(3u, account.Balances.size());
				  EXPECT_EQ(3u, account.Balances.lockedBalances().size());
				  EXPECT_EQ(3u, account.Balances.balances().size());
				  EXPECT_EQ(100, account.Balances.get(MosaicId(72)));
				  EXPECT_EQ(100, account.Balances.getLocked(MosaicId(72)));
				  EXPECT_EQ(100, account.Balances.get(MosaicId(172)));
				  EXPECT_EQ(100, account.Balances.getLocked(MosaicId(172)));
				  EXPECT_EQ(100, account.Balances.get(MosaicId(200)));
				  EXPECT_EQ(100, account.Balances.getLocked(MosaicId(200)));
				});

		auto newContext = context.alterMode(NotifyMode::Rollback);
		// Act: rollback and verify
		VerifyDefaultCacheStateAfterRollback(notification, signer, newContext, mosaics);
	}

	TEST(TEST_CLASS, ObserverLocksAllOfMosaicOnCommit) {
		// Arrange
		auto signer = test::GenerateRandomByteArray<Key>();
		auto notification = CreateTransferNotification(signer, { { UnresolvedMosaicId(72), UnresolvedAmount(200) } }, BlockDuration(0), model::LockFundAction::Lock);
		auto context = PrepareContext(Height{444}, 256, BlockDuration(10));
		// Act: add it
		RunChildTest(
				notification,
				context,
				PrepareDefaultAccount(signer),
				[&signer](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta) {
				  // Assert: balances was locked but no unlocking record was added.All funds are no locked so regular balance is empty.
				  auto account = accountStateCacheDelta.find(signer).get();
				  auto lockFundKeyRecord = lockFundCacheDelta.find(signer).tryGet();
				  EXPECT_FALSE(lockFundKeyRecord);
				  EXPECT_EQ(1u, account.Balances.size());
				  EXPECT_EQ(1u, account.Balances.lockedBalances().size());
				  EXPECT_EQ(0u, account.Balances.balances().size());
				  EXPECT_EQ(0, account.Balances.get(MosaicId(72)));
				  EXPECT_EQ(200, account.Balances.getLocked(MosaicId(72)));
				});

		auto newContext = context.alterMode(NotifyMode::Rollback);
		// Act: rollback and verify
		VerifyDefaultCacheStateAfterRollback(notification, signer, newContext);
	}

	TEST(TEST_CLASS, ObserverLocksAllOfMultipleMosaicsOnCommit) {
		// Arrange
		auto signer = test::GenerateRandomByteArray<Key>();
		auto notification = CreateTransferNotification(signer, {
															   { UnresolvedMosaicId(72), UnresolvedAmount(200) },
															   { UnresolvedMosaicId(172), UnresolvedAmount(100) },
															   { UnresolvedMosaicId(200), UnresolvedAmount(200) }
													   },
													   BlockDuration(0), model::LockFundAction::Lock);
		auto context = PrepareContext(Height{444}, 256, BlockDuration(10));
		std::map<MosaicId, Amount> mosaics = { { MosaicId(72), Amount(200) }, { MosaicId(172), Amount(100) }, { MosaicId(200), Amount(200) } };
		// Act: add it
		RunChildTest(
				notification,
				context,
				PrepareDefaultAccount(signer, { { MosaicId(72), Amount(200) }, { MosaicId(172), Amount(200) }, { MosaicId(200), Amount(200) } }),
				[&signer](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta) {
				  // Assert: balances was locked but no unlocking record was added.Some balance still persists in the regular balance and the remainder is now locked
				  auto account = accountStateCacheDelta.find(signer).get();
				  auto lockFundKeyRecord = lockFundCacheDelta.find(signer).tryGet();
				  EXPECT_FALSE(lockFundKeyRecord);
				  EXPECT_EQ(3u, account.Balances.size());
				  EXPECT_EQ(3u, account.Balances.lockedBalances().size());
				  EXPECT_EQ(1u, account.Balances.balances().size());
				  EXPECT_EQ(0, account.Balances.get(MosaicId(72)));
				  EXPECT_EQ(200, account.Balances.getLocked(MosaicId(72)));
				  EXPECT_EQ(100, account.Balances.get(MosaicId(172)));
				  EXPECT_EQ(100, account.Balances.getLocked(MosaicId(172)));
				  EXPECT_EQ(0, account.Balances.get(MosaicId(200)));
				  EXPECT_EQ(200, account.Balances.getLocked(MosaicId(200)));
				});

		auto newContext = context.alterMode(NotifyMode::Rollback);
		// Act: rollback and verify
		VerifyDefaultCacheStateAfterRollback(notification, signer, newContext, mosaics);
	}

	TEST(TEST_CLASS, ObserverLocksAllOfMosaicOnCommitAndGeneratesUnlockRecord) {
		// Arrange: create a child namespace with a root parent
		auto signer = test::GenerateRandomByteArray<Key>();
		auto notification = CreateTransferNotification(signer, { { UnresolvedMosaicId(72), UnresolvedAmount(200) } }, BlockDuration(10), model::LockFundAction::Lock);
		auto context = PrepareContext(Height{444}, 256, BlockDuration(10));

		// Act: add it
		RunChildTest(
		notification,
		context,
		PrepareDefaultAccount(signer),
		[&signer](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta) {
		  // Assert: balances was locked but no unlocking record was added.All funds are no locked so regular balance is empty.
		  auto account = accountStateCacheDelta.find(signer).get();
		  auto lockFundKeyRecord = lockFundCacheDelta.find(signer).tryGet();
		  auto lockFundHeightRecord = lockFundCacheDelta.find(Height(10)).tryGet();

		  //Validate unlocking records
		  EXPECT_TRUE(lockFundKeyRecord);
		  EXPECT_TRUE(lockFundHeightRecord);
		  EXPECT_EQ(lockFundKeyRecord->LockFundRecords.size(), lockFundHeightRecord->LockFundRecords.size());
		  EXPECT_EQ(lockFundKeyRecord->LockFundRecords.size(), 1);
		  EXPECT_TRUE(lockFundHeightRecord->LockFundRecords.find(signer)->second.Active());
		  EXPECT_FALSE(lockFundHeightRecord->LockFundRecords.find(signer)->second.InactiveRecords.size());
		  EXPECT_TRUE(lockFundKeyRecord->LockFundRecords.find(Height(10))->second.Active());
		  EXPECT_FALSE(lockFundKeyRecord->LockFundRecords.find(Height(10))->second.InactiveRecords.size());
		  EXPECT_EQ(lockFundKeyRecord->Identifier, signer);
		  EXPECT_EQ(lockFundHeightRecord->Identifier, Height(10));

		  //Validate unlocking amounts

		  auto keyRecord = lockFundKeyRecord->LockFundRecords.find(Height(10))->second.Get();
		  auto heightRecord = lockFundHeightRecord->LockFundRecords.find(signer)->second.Get();
		  EXPECT_EQ(heightRecord.size(), 1);
		  EXPECT_EQ(heightRecord.find(MosaicId(72))->second, 200);
		  EXPECT_EQ(keyRecord.size(), 1);
		  EXPECT_EQ(keyRecord.find(MosaicId(72))->second, 200);

		  //Validate Balances
		  EXPECT_EQ(1u, account.Balances.size());
		  EXPECT_EQ(1u, account.Balances.lockedBalances().size());
		  EXPECT_EQ(0u, account.Balances.balances().size());
		  EXPECT_EQ(0, account.Balances.get(MosaicId(72)));
		  EXPECT_EQ(200, account.Balances.getLocked(MosaicId(72)));
		});
		// Act: rollback and verify
		auto newContext = context.alterMode(NotifyMode::Rollback);
		VerifyDefaultCacheStateAfterRollback(notification, signer, newContext);
	}

	TEST(TEST_CLASS, ObserverLocksAllOfMultipleMosaicsOnCommitAndGeneratesUnlockRecord) {
		// Arrange: create a child namespace with a root parent
		auto signer = test::GenerateRandomByteArray<Key>();
		auto notification = CreateTransferNotification(signer, {
				{ UnresolvedMosaicId(72), UnresolvedAmount(200) },
				{ UnresolvedMosaicId(172), UnresolvedAmount(100) },
				{ UnresolvedMosaicId(200), UnresolvedAmount(200) }
		}, BlockDuration(10), model::LockFundAction::Lock);
		auto context = PrepareContext(Height{444}, 256, BlockDuration(10));
		std::map<MosaicId, Amount> mosaics = { { MosaicId(72), Amount(200) }, { MosaicId(172), Amount(100) }, { MosaicId(200), Amount(200) } };

		// Act: add it
		RunChildTest(
				notification,
				context,
				PrepareDefaultAccount(signer, mosaics),
				[&signer, &mosaics](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta) {
				  // Assert: balances was locked but no unlocking record was added.All funds are no locked so regular balance is empty.
				  auto account = accountStateCacheDelta.find(signer).get();
				  auto lockFundKeyRecord = lockFundCacheDelta.find(signer).tryGet();
				  auto lockFundHeightRecord = lockFundCacheDelta.find(Height(10)).tryGet();

				  //Validate unlocking records
				  EXPECT_TRUE(lockFundKeyRecord);
				  EXPECT_TRUE(lockFundHeightRecord);
				  EXPECT_EQ(lockFundKeyRecord->LockFundRecords.size(), lockFundHeightRecord->LockFundRecords.size());
				  EXPECT_EQ(lockFundKeyRecord->LockFundRecords.size(), 1);
				  EXPECT_TRUE(lockFundHeightRecord->LockFundRecords.find(signer)->second.Active());
				  EXPECT_FALSE(lockFundHeightRecord->LockFundRecords.find(signer)->second.InactiveRecords.size());
				  EXPECT_TRUE(lockFundKeyRecord->LockFundRecords.find(Height(10))->second.Active());
				  EXPECT_FALSE(lockFundKeyRecord->LockFundRecords.find(Height(10))->second.InactiveRecords.size());
				  EXPECT_EQ(lockFundKeyRecord->Identifier, signer);
				  EXPECT_EQ(lockFundHeightRecord->Identifier, Height(10));

				  //Validate unlocking amounts
				  auto keyRecord = lockFundKeyRecord->LockFundRecords.find(Height(10))->second.Get();
				  auto heightRecord = lockFundHeightRecord->LockFundRecords.find(signer)->second.Get();
				  EXPECT_EQ(heightRecord.size(), mosaics.size());
				  EXPECT_EQ(keyRecord.size(), mosaics.size());
				  for(auto mosaic : mosaics)
				  {
					  EXPECT_EQ(heightRecord.find(mosaic.first)->second, mosaic.second);
					  EXPECT_EQ(keyRecord.find(mosaic.first)->second, mosaic.second);
				  }

				  //Validate Balances

				  EXPECT_EQ(mosaics.size(), account.Balances.size());
				  EXPECT_EQ(mosaics.size(), account.Balances.lockedBalances().size());
				  EXPECT_EQ(0u, account.Balances.balances().size());

				  for(auto mosaic : mosaics)
				  {
					  EXPECT_EQ(0, account.Balances.get(mosaic.first));
					  EXPECT_EQ(mosaic.second, account.Balances.getLocked(mosaic.first));
				  }

				});
		// Act: rollback and verify
		auto newContext = context.alterMode(NotifyMode::Rollback);
		VerifyDefaultCacheStateAfterRollback(notification, signer, newContext, mosaics);
	}

	// endregion

	// region unlocking
	TEST(TEST_CLASS, ObserverUnlockingMultipleMosaicsWithZeroDurationGeneratesUnlockRecordWithMinimumDuration) {
		// Arrange: create a child namespace with a root parent
		auto signer = test::GenerateRandomByteArray<Key>();
		auto notification = CreateTransferNotification(signer, {
				{ UnresolvedMosaicId(72), UnresolvedAmount(200) },
				{ UnresolvedMosaicId(172), UnresolvedAmount(100) },
				{ UnresolvedMosaicId(200), UnresolvedAmount(200) }
		}, BlockDuration(0), model::LockFundAction::Unlock);
		auto context = PrepareContext(Height{444}, 256, BlockDuration(10));
		std::map<MosaicId, Amount> mosaics = { { MosaicId(72), Amount(200) }, { MosaicId(172), Amount(100) }, { MosaicId(200), Amount(200) } };

		// Act: add it
		RunChildTest(
				notification,
				context,
				PrepareDefaultAccount(signer, mosaics), //To be able to use the test rollback function
				[&signer, &mosaics](cache::LockFundCacheDelta& lockFundCacheDelta, cache::AccountStateCacheDelta& accountStateCacheDelta) {
				  // Assert: balances was locked but no unlocking record was added.All funds are no locked so regular balance is empty.
				  auto account = accountStateCacheDelta.find(signer).get();
				  auto lockFundKeyRecord = lockFundCacheDelta.find(signer).tryGet();
				  auto lockFundHeightRecord = lockFundCacheDelta.find(Height(10)).tryGet();

				  //Validate unlocking records
				  EXPECT_TRUE(lockFundKeyRecord);
				  EXPECT_TRUE(lockFundHeightRecord);
				  EXPECT_EQ(lockFundKeyRecord->LockFundRecords.size(), lockFundHeightRecord->LockFundRecords.size());
				  EXPECT_EQ(lockFundKeyRecord->LockFundRecords.size(), 1);
				  EXPECT_TRUE(lockFundHeightRecord->LockFundRecords.find(signer)->second.Active());
				  EXPECT_FALSE(lockFundHeightRecord->LockFundRecords.find(signer)->second.InactiveRecords.size());
				  EXPECT_TRUE(lockFundKeyRecord->LockFundRecords.find(Height(10))->second.Active());
				  EXPECT_FALSE(lockFundKeyRecord->LockFundRecords.find(Height(10))->second.InactiveRecords.size());
				  EXPECT_EQ(lockFundKeyRecord->Identifier, signer);
				  EXPECT_EQ(lockFundHeightRecord->Identifier, Height(10));

				  //Validate unlocking amounts
				  auto keyRecord = lockFundKeyRecord->LockFundRecords.find(Height(10))->second.Get();
				  auto heightRecord = lockFundHeightRecord->LockFundRecords.find(signer)->second.Get();
				  EXPECT_EQ(heightRecord.size(), mosaics.size());
				  EXPECT_EQ(keyRecord.size(), mosaics.size());
				  for(auto mosaic : mosaics)
				  {
					  EXPECT_EQ(heightRecord.find(mosaic.first)->second, mosaic.second);
					  EXPECT_EQ(keyRecord.find(mosaic.first)->second, mosaic.second);
				  }

				});
		// Act: rollback and verify
		auto newContext = context.alterMode(NotifyMode::Rollback);
		VerifyDefaultCacheStateAfterRollback(notification, signer, newContext, mosaics);
	}
	// endregion
}}
