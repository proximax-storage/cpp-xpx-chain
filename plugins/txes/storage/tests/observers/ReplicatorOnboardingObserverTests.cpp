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
        using ObserverTestContext = test::ObserverTestContextT<test::ReplicatorCacheFactory>;
        using Notification = model::ReplicatorOnboardingNotification<1>;

        const auto Replicator_Key_Collector = std::make_shared<cache::ReplicatorKeyCollector>();
        constexpr auto Current_Height = Height(25);

        struct ReplicatorValues {
            public:
                explicit ReplicatorValues()
                    : PublicKey(test::GenerateRandomByteArray<Key>())
                    , Capacity(test::GenerateRandomValue<Amount>())
                {}

            public:
                Key PublicKey;
                Amount Capacity;
        };

        state::ReplicatorEntry CreateEntry(const ReplicatorValues& values) {
            state::ReplicatorEntry entry(values.PublicKey);
            entry.setCapacity(values.Capacity);

            return entry;
        }
    }

    TEST(TEST_CLASS, ReplicatorOnboarding_Commit) {
        // Arrange:
        ObserverTestContext context(NotifyMode::Commit, Current_Height);
        ReplicatorValues values;
        Notification notification(values.PublicKey, values.Capacity);
        auto pObserver = CreateReplicatorOnboardingObserver();
        auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();

        // Populate cache.
        replicatorCache.insert(CreateEntry(values));

        // Act:
        test::ObserveNotification(*pObserver, notification, context);

        // Assert:
        auto driveIter = replicatorCache.find(values.PublicKey);
        auto& actualEntry = driveIter.get();
        test::AssertEqualReplicatorData(CreateEntry(values), actualEntry);
    }
}}