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

	DEFINE_COMMON_OBSERVER_TESTS(ReplicatorsCleanup, Liquidity_Provider_Ptr)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::StorageCacheFactory>;
        using Notification = model::ReplicatorsCleanupNotification<1>;

        void RunTest(NotifyMode mode) {
            // Arrange:
            ObserverTestContext context(mode, Height(1));
			std::vector<Key> replicatorKeys{
				Key({ 1 }),
				Key({ 3 }),
				Key({ 5 }),
				Key({ 7 }),
				Key({ 9 }),
			};
            Notification notification(replicatorKeys.size(), replicatorKeys.data());
			auto& cache = context.cache().sub<cache::ReplicatorCache>();
			for (uint8_t i = 1; i <= 10; ++i)
				cache.insert(state::ReplicatorEntry(Key({ i })));
			auto pObserver = CreateReplicatorsCleanupObserver(Liquidity_Provider_Ptr);

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

    TEST(TEST_CLASS, ReplicatorsCleanup_Commit) {
        // Assert:
        RunTest(NotifyMode::Commit);
    }

    TEST(TEST_CLASS, ReplicatorsCleanup_Rollback) {
        // Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback), catapult_runtime_error);
    }
}}