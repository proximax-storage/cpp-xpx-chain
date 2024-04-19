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

#define TEST_CLASS ReplicatorNodeBootKeyObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(ReplicatorNodeBootKey,)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::StorageCacheFactory>;
        using Notification = model::ReplicatorNodeBootKeyNotification<1>;

        void RunTest(NotifyMode mode) {
            // Arrange:
            ObserverTestContext context(mode, Height(1));
			Key replicatorKey = test::GenerateRandomByteArray<Key>();
			Key nodeBootKey = test::GenerateRandomByteArray<Key>();
            Notification notification(replicatorKey, nodeBootKey);
			auto& cache = context.cache().sub<cache::BootKeyReplicatorCache>();
			auto pObserver = CreateReplicatorNodeBootKeyObserver();

			// Sanity:
			ASSERT_FALSE(cache.contains(nodeBootKey));

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
     		auto iter = cache.find(nodeBootKey);
			const auto& actualEntry = iter.get();
			auto expectedEntry = test::CreateBootKeyReplicatorEntry(nodeBootKey, replicatorKey);
			test::AssertEqualBootKeyReplicatorData(expectedEntry, actualEntry);
        }
    }

    TEST(TEST_CLASS, ReplicatorNodeBootKey_Commit) {
        // Assert:
        RunTest(NotifyMode::Commit);
    }

    TEST(TEST_CLASS, ReplicatorNodeBootKey_Rollback) {
        // Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback), catapult_runtime_error);
    }
}}