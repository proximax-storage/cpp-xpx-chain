/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/StorageTestUtils.h"
#include "catapult/model/StorageNotifications.h"
#include "src/observers/Observers.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS ReplicatorOnboardingObserverTests

    DEFINE_COMMON_OBSERVER_TESTS(ReplicatorOnboarding,)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::StorageCacheFactory>;
        using Notification = model::ReplicatorOnboardingNotification<1>;

        const Key Public_Key = test::GenerateRandomByteArray<Key>();
        constexpr auto Capacity = Amount(50);
        constexpr auto Replicator_Count = 10;
        constexpr auto Current_Height = Height(25);

        state::ReplicatorEntry CreateReplicatorEntry() {
            state::ReplicatorEntry entry(Public_Key);
            entry.setCapacity(Capacity);

            return entry;
        }

        void RunTest(NotifyMode mode, const state::ReplicatorEntry& expectedReplicatorEntry, const Height& currentHeight) {
            // Arrange:
            ObserverTestContext context(mode, currentHeight);
            Notification notification(
				expectedReplicatorEntry.key(),
				expectedReplicatorEntry.capacity());
            auto pObserver = CreateReplicatorOnboardingObserver();
        	auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
     		auto replicatorIter = replicatorCache.find(expectedReplicatorEntry.key());
			const auto& actualReplicatorEntry = replicatorIter.get();
			test::AssertEqualReplicatorData(expectedReplicatorEntry, actualReplicatorEntry);
        }
    }

    TEST(TEST_CLASS, ReplicatorOnboarding_Commit) {
        // Arrange:
        const auto expectedReplicatorEntry = CreateReplicatorEntry();

        // Assert:
        RunTest(NotifyMode::Commit, expectedReplicatorEntry, Current_Height);
    }

    TEST(TEST_CLASS, ReplicatorOnboarding_Rollback) {
        // Arrange:
		const auto expectedReplicatorEntry = CreateReplicatorEntry();

        // Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, expectedReplicatorEntry, Current_Height), catapult_runtime_error);
    }
}}