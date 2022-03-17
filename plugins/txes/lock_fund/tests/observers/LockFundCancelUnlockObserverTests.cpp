/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#include "src/cache/LockFundCache.h"
#include "src/config/LockFundConfiguration.h"
#include "src/observers/Observers.h"
#include "tests/test/LockFundCacheFactory.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS LockFundCancelUnlockObserverTests

		using ObserverTestContext = test::ObserverTestContextT<test::LockFundCacheFactory>;

		DEFINE_COMMON_OBSERVER_TESTS(LockFundCancelUnlock, )

		namespace {

			struct RecordDescriptor {
				Height height;
				Key key;
				std::map<MosaicId, Amount> mosaics = { { MosaicId(72), Amount(200) } };
			};

			model::LockFundCancelUnlockNotification<1>
					CreateLockFundCancelUnlockNotification(const RecordDescriptor& descriptor) {
				return model::LockFundCancelUnlockNotification<1>(descriptor.key, descriptor.height);
			}

			auto PrepareDefaultUnlockRecords(const std::vector<RecordDescriptor> descriptors) {
				return [&descriptors](cache::LockFundCacheDelta& lockFundCacheDelta) {
					for (auto& descriptor : descriptors) {
						lockFundCacheDelta.insert(descriptor.key, descriptor.height, descriptor.mosaics);
					}
				};
			}

			template<typename TSeedCacheFunc, typename TCheckCacheFunc>
			void RunCancelUnlockTest(
					const model::LockFundCancelUnlockNotification<1>& notification,
					ObserverTestContext& context,
					TSeedCacheFunc seedCache,
					TCheckCacheFunc checkCache) {
				// Arrange:
				auto pObserver = CreateLockFundCancelUnlockObserver();

				// - seed the cache
				auto& lockFundCacheDelta = context.cache().sub<cache::LockFundCache>();
				seedCache(lockFundCacheDelta);
				// Act:
				test::ObserveNotification(*pObserver, notification, context);

				// Assert: check the cache
				checkCache(lockFundCacheDelta);
			}

			template<typename TSeedCacheFunc, typename TCheckCacheFunc>
			void RunCancelUnlockTestStream(
					const std::vector<model::LockFundCancelUnlockNotification<1>>& notifications,
					ObserverTestContext& context,
					TSeedCacheFunc seedCache,
					TCheckCacheFunc checkCache) {
				// Arrange:
				auto pObserver = CreateLockFundCancelUnlockObserver();

				// - seed the cache
				auto& lockFundCacheDelta = context.cache().sub<cache::LockFundCache>();
				seedCache(lockFundCacheDelta);
				// Act:
				if (context.observerContext().Mode == NotifyMode::Commit) {
					for (auto& notification : notifications) {
						test::ObserveNotification(*pObserver, notification, context);
					}
				} else {
					for (auto itNotification = notifications.rbegin(); itNotification != notifications.rend();
						 ++itNotification) {
						test::ObserveNotification(*pObserver, *itNotification, context);
					}
				}

				// Assert: check the cache
				checkCache(lockFundCacheDelta);
			}

			auto PrepareConfiguration(uint8_t maxMosaicsSize, BlockDuration unlockCooldown) {
				auto pluginConfig = config::LockFundConfiguration::Uninitialized();
				pluginConfig.MaxMosaicsSize = maxMosaicsSize;
				pluginConfig.MinRequestUnlockCooldown = unlockCooldown;
				test::MutableBlockchainConfiguration mutableConfig;
				mutableConfig.Network.SetPluginConfiguration(pluginConfig);
				return mutableConfig.ToConst();
			}

			auto PrepareContext(
					Height height,
					uint8_t maxMosaicsSize,
					BlockDuration unlockCooldown,
					NotifyMode mode = NotifyMode::Commit) {
				return ObserverTestContext(mode, height, PrepareConfiguration(maxMosaicsSize, unlockCooldown));
			}

			auto CheckCacheDefault(const std::vector<RecordDescriptor>& descriptors) {
				return [&descriptors](cache::LockFundCacheDelta& lockFundCacheDelta) {
					std::unordered_map<Key, int, utils::ArrayHasher<Key>> keyCount;
					std::unordered_map<Height, int, utils::BaseValueHasher<Height>> heightCount;
					for (auto& descriptor : descriptors) {
						keyCount[descriptor.key]++;
						heightCount[descriptor.height]++;
					}
					for (auto& key : keyCount) {
						auto lockFundKeyRecord = lockFundCacheDelta.find(key.first).tryGet();
						EXPECT_TRUE(lockFundKeyRecord);
						EXPECT_EQ(lockFundKeyRecord->LockFundRecords.size(), key.second);
					}

					for (auto& height : heightCount) {
						auto lockFundHeightRecord = lockFundCacheDelta.find(height.first).tryGet();
						EXPECT_TRUE(lockFundHeightRecord);
						EXPECT_EQ(lockFundHeightRecord->LockFundRecords.size(), height.second);
					}
				};
			}
			auto VerifyDefaultCacheStateAfterRollback(
					const model::LockFundCancelUnlockNotification<1>& notification,
					const std::vector<RecordDescriptor>& descriptors,
					ObserverTestContext& context) {
				RunCancelUnlockTest(
						notification,
						context,
						[](cache::LockFundCacheDelta& lockFundCacheDelta) {},
						CheckCacheDefault(descriptors));
			}
			auto VerifyDefaultCacheStateAfterRollback(
					const std::vector<model::LockFundCancelUnlockNotification<1>>& notifications,
					const std::vector<RecordDescriptor>& descriptors,
					ObserverTestContext& context) {
				RunCancelUnlockTestStream(
						notifications,
						context,
						[](cache::LockFundCacheDelta& lockFundCacheDelta) {},
						CheckCacheDefault(descriptors));
			}
		}

		// region locking

		TEST(TEST_CLASS, ObserverCancelsExistingUnlock) {
			// Arrange
			std::vector<RecordDescriptor> descriptors = { { Height(20), test::GenerateRandomByteArray<Key>() } };
			auto notification = CreateLockFundCancelUnlockNotification(descriptors[0]);
			auto context = PrepareContext(Height { 10 }, 256, BlockDuration(10));
			// Act: add it
			RunCancelUnlockTest(
					notification,
					context,
					PrepareDefaultUnlockRecords(descriptors),
					[&descriptors](cache::LockFundCacheDelta& lockFundCacheDelta) {
						// Assert
						auto& descriptor = descriptors[0];
						auto lockFundKeyRecord = lockFundCacheDelta.find(descriptor.key).tryGet();
						auto lockFundHeightRecord = lockFundCacheDelta.find(descriptor.height).tryGet();
						// Validate unlocking records
						EXPECT_TRUE(lockFundKeyRecord);
						EXPECT_TRUE(lockFundHeightRecord);
						EXPECT_EQ(
								lockFundKeyRecord->LockFundRecords.size(),
								lockFundHeightRecord->LockFundRecords.size());
						EXPECT_EQ(lockFundKeyRecord->LockFundRecords.size(), 1);

						EXPECT_FALSE(lockFundHeightRecord->LockFundRecords.find(descriptor.key)
											 ->second.Active());
						EXPECT_EQ(
								1,
								lockFundHeightRecord->LockFundRecords.find(descriptor.key)
										->second.InactiveRecords.size());

						EXPECT_FALSE(
								lockFundKeyRecord->LockFundRecords.find(descriptor.height)->second.Active());
						EXPECT_EQ(
								1, lockFundKeyRecord->LockFundRecords.find(Height(descriptor.height))->second.InactiveRecords.size());

						EXPECT_EQ(lockFundKeyRecord->Identifier, descriptor.key);
						EXPECT_EQ(lockFundHeightRecord->Identifier, descriptor.height);

						// Validate unlocking amounts
						auto keyRecord =
								lockFundKeyRecord->LockFundRecords.find(descriptor.height)->second.InactiveRecords[0];
						auto heightRecord =
								lockFundHeightRecord->LockFundRecords.find(descriptor.key)->second.InactiveRecords[0];

						EXPECT_EQ(heightRecord.size(), 1);
						EXPECT_EQ(keyRecord.size(), 1);
					});

			auto newContext = context.alterMode(NotifyMode::Rollback);
			// Act: rollback and verify
			VerifyDefaultCacheStateAfterRollback(notification, descriptors, newContext);
		}

		TEST(TEST_CLASS, ObserverCancelsMultipleUnlocksSameKey) {
			// Arrange

			auto signer = test::GenerateRandomByteArray<Key>();
			std::vector<RecordDescriptor> descriptors = { { Height(10),  signer}, { Height(13),  signer} };
			std::vector<model::LockFundCancelUnlockNotification<1>> notifications = { CreateLockFundCancelUnlockNotification(descriptors[0]), CreateLockFundCancelUnlockNotification(descriptors[1])};
			auto context = PrepareContext(Height { 444 }, 256, BlockDuration(10));
			// Act: add it
			RunCancelUnlockTestStream(
					notifications,
					context,
					PrepareDefaultUnlockRecords(descriptors),
					[&descriptors](cache::LockFundCacheDelta& lockFundCacheDelta) {
					  // Assert
					  auto& descriptor = descriptors[0];
					  auto lockFundKeyRecord = lockFundCacheDelta.find(descriptor.key).tryGet();
					  auto lockFundHeightRecord = lockFundCacheDelta.find(descriptor.height).tryGet();
					  // Validate unlocking records
					  EXPECT_TRUE(lockFundKeyRecord);
					  EXPECT_TRUE(lockFundHeightRecord);
					  EXPECT_EQ(lockFundHeightRecord->LockFundRecords.size(), 1);
					  EXPECT_EQ(lockFundKeyRecord->LockFundRecords.size(), 2);

					  EXPECT_FALSE(lockFundHeightRecord->LockFundRecords.find(descriptor.key)
										   ->second.Active());
					  EXPECT_EQ(
							  1,
							  lockFundHeightRecord->LockFundRecords.find(descriptor.key)
									  ->second.InactiveRecords.size());

					  EXPECT_FALSE(
							  lockFundKeyRecord->LockFundRecords.find(Height(10))->second.Active());
					  EXPECT_EQ(
							  1, lockFundKeyRecord->LockFundRecords.find(Height(10))->second.InactiveRecords.size());

					  EXPECT_EQ(lockFundKeyRecord->Identifier, descriptor.key);
					  EXPECT_EQ(lockFundHeightRecord->Identifier, descriptor.height);

					  // Validate unlocking amounts
					  auto keyRecord =
							  lockFundKeyRecord->LockFundRecords.find(descriptor.height)->second.InactiveRecords[0];
					  auto heightRecord =
							  lockFundHeightRecord->LockFundRecords.find(descriptor.key)->second.InactiveRecords[0];

					  EXPECT_EQ(heightRecord.size(), 1);
					  EXPECT_EQ(keyRecord.size(), 1);
					});

			auto newContext = context.alterMode(NotifyMode::Rollback);
			// Act: rollback and verify
			VerifyDefaultCacheStateAfterRollback(notifications, descriptors, newContext);
		}
}}