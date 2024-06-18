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
			auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();
			state::ReplicatorEntry replicatorEntry(replicatorKey);
			replicatorEntry.setVersion(2);
			replicatorCache.insert(replicatorEntry);
			auto& bootKeyReplicatorCache = context.cache().sub<cache::BootKeyReplicatorCache>();
			auto pObserver = CreateReplicatorNodeBootKeyObserver();

			// Sanity:
			ASSERT_FALSE(bootKeyReplicatorCache.contains(nodeBootKey));

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
			replicatorEntry.setNodeBootKey(nodeBootKey);
			auto replicatorIter = replicatorCache.find(replicatorKey);
			const auto& actualReplicatorEntry = replicatorIter.get();
			test::AssertEqualReplicatorData(replicatorEntry, actualReplicatorEntry);

     		auto bootKeyIter = bootKeyReplicatorCache.find(nodeBootKey);
			const auto& actualBootKeyReplicatorEntry = bootKeyIter.get();
			auto expectedBootKeyReplicatorEntry = test::CreateBootKeyReplicatorEntry(nodeBootKey, replicatorKey);
			test::AssertEqualBootKeyReplicatorData(expectedBootKeyReplicatorEntry, actualBootKeyReplicatorEntry);
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