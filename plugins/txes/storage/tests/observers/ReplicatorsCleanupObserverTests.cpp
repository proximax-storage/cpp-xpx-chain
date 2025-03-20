/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/StorageTestUtils.h"
#include "src/observers/Observers.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS ReplicatorsCleanupObserverTests

	const std::unique_ptr<observers::LiquidityProviderExchangeObserver>  Liquidity_Provider_Ptr = std::make_unique<test::LiquidityProviderExchangeObserverImpl>();

	DEFINE_COMMON_OBSERVER_TESTS(ReplicatorsCleanupV1, Liquidity_Provider_Ptr)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::StorageCacheFactory>;

        void RunV1Test(NotifyMode mode) {
            // Arrange:
            ObserverTestContext context(mode, Height(1));
			std::vector<Key> replicatorKeys{
				Key({ 1 }),
				Key({ 3 }),
				Key({ 5 }),
				Key({ 7 }),
				Key({ 9 }),
			};
			model::ReplicatorsCleanupNotification<1> notification(replicatorKeys.size(), replicatorKeys.data());
			auto& cache = context.cache().sub<cache::ReplicatorCache>();
			for (uint8_t i = 1; i <= 10; ++i)
				cache.insert(state::ReplicatorEntry(Key({ i })));
			auto pObserver = CreateReplicatorsCleanupV1Observer(Liquidity_Provider_Ptr);

            // Sanity
			for (uint8_t i = 1; i <= 10; ++i)
				ASSERT_TRUE(cache.contains(Key({ i })));

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
			for (const auto& replicatorKey : replicatorKeys)
				ASSERT_FALSE(cache.contains(replicatorKey));

			for (uint8_t i = 2; i <= 10; i += 2)
				ASSERT_TRUE(cache.contains(Key({ i })));
        }
    }

    TEST(TEST_CLASS, ReplicatorsCleanupV1_Commit) {
        // Assert:
        RunV1Test(NotifyMode::Commit);
    }

    TEST(TEST_CLASS, ReplicatorsCleanupV1_Rollback) {
        // Assert
		EXPECT_THROW(RunV1Test(NotifyMode::Rollback), catapult_runtime_error);
    }

	DEFINE_COMMON_OBSERVER_TESTS(ReplicatorsCleanupV2)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::StorageCacheFactory>;

        void RunV2Test(NotifyMode mode) {
            // Arrange:
            ObserverTestContext context(mode, Height(1));
			std::vector<Key> replicatorKeys{
				Key({ 1 }),
				Key({ 3 }),
				Key({ 5 }),
				Key({ 7 }),
				Key({ 9 }),
			};
			model::ReplicatorsCleanupNotification<2> notification(replicatorKeys.size(), replicatorKeys.data());
			auto& cache = context.cache().sub<cache::ReplicatorCache>();
			for (uint8_t i = 1; i <= 10; ++i) {
				auto entry = state::ReplicatorEntry(Key({ i }));
				for (uint8_t j = 1; j <= 5; ++j)
					entry.drives().emplace(Key({ static_cast<uint8_t>(10 + j) }), state::DriveInfo{});
				cache.insert(entry);
			}
			auto pObserver = CreateReplicatorsCleanupV2Observer();

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
			for (uint8_t i = 1; i <= 10; ++i) {
				auto replicatorKey = Key({ i });
				auto iter = cache.find(replicatorKey);
				const auto& entry = iter.get();
				auto driveCount = (1 == i % 2) ? 0u : 5u;
				ASSERT_EQ(driveCount, entry.drives().size());
			}
        }
    }

    TEST(TEST_CLASS, ReplicatorsCleanupV2_Commit) {
        // Assert:
        RunV2Test(NotifyMode::Commit);
    }

    TEST(TEST_CLASS, ReplicatorsCleanupV2_Rollback) {
        // Assert
		EXPECT_THROW(RunV2Test(NotifyMode::Rollback), catapult_runtime_error);
    }
}}