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

        constexpr auto Current_Height = Height(25);

        struct ReplicatorValues {
            public:
                explicit ReplicatorValues()
                    : PublicKey(test::GenerateRandomByteArray<Key>())
                    , BlsKey(test::GenerateRandomByteArray<BLSPublicKey>())
                    , Capacity(test::GenerateRandomValue<Amount>())
                {}

            public:
                Key PublicKey;
                BLSPublicKey BlsKey;
                Amount Capacity;
        };

        state::ReplicatorEntry CreateReplicatorEntry(const ReplicatorValues& values) {
            state::ReplicatorEntry entry(values.PublicKey);
            entry.setCapacity(values.Capacity);

            return entry;
        }

        void RunTest(NotifyMode mode, const ReplicatorValues& values, const Height& currentHeight) {
            // Arrange:
            ObserverTestContext context(mode, currentHeight);
            Notification notification(values.PublicKey, values.BlsKey, values.Capacity);
            auto pObserver = CreateReplicatorOnboardingObserver();
        	auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();
            
            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
     		auto replicatorIter = replicatorCache.find(values.PublicKey);
			const auto &actualEntry = replicatorIter.get();
			test::AssertEqualReplicatorData(CreateReplicatorEntry(values), actualEntry);

        }
    }

    TEST(TEST_CLASS, ReplicatorOnboarding_Commit) {
        // Arrange:
        ReplicatorValues values;

        // Assert:
        RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, ReplicatorOnboarding_Rollback) {
        // Arrange:
        ReplicatorValues values;

        // Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values, Current_Height), catapult_runtime_error);
    }
}}