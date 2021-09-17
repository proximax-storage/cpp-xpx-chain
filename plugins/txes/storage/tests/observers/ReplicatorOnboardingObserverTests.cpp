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

        const Key Public_Key = test::GenerateRandomByteArray<Key>();
        const BLSPublicKey Bls_Key = test::GenerateRandomByteArray<BLSPublicKey>();
        constexpr auto Capacity = Amount(50);
        constexpr auto Replicator_Count = 10;
        constexpr auto Current_Height = Height(25);

        state::ReplicatorEntry CreateReplicatorEntry() {
            state::ReplicatorEntry entry(Public_Key);
            entry.setCapacity(Capacity);
            entry.setBlsKey(Bls_Key);

            return entry;
        }

        state::BlsKeysEntry CreateBlsKeyEntry() {
            state::BlsKeysEntry entry(Bls_Key);
            entry.setKey(Public_Key);

            return entry;
        }

        struct CacheValues {
            public:
			    explicit CacheValues()
                    : ReplicatorEntry(Key())
                    , BlsKeyEntry(BLSPublicKey())
                {}

            public:
                state::ReplicatorEntry ReplicatorEntry;
                state::BlsKeysEntry BlsKeyEntry;
        };

        void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight) {
            // Arrange:
            ObserverTestContext context(mode, currentHeight);
            Notification notification(
                values.ReplicatorEntry.key(),
                values.ReplicatorEntry.blsKey(),
                values.ReplicatorEntry.capacity());
            auto pObserver = CreateReplicatorOnboardingObserver();
        	auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();
            auto& blsKeysCache = context.cache().sub<cache::BlsKeysCache>();
            
            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
     		auto replicatorIter = replicatorCache.find(values.ReplicatorEntry.key());
			const auto &replicatorEntry = replicatorIter.get();
			test::AssertEqualReplicatorData(values.ReplicatorEntry, replicatorEntry);

     		auto BlsKeyIter = blsKeysCache.find(values.BlsKeyEntry.blsKey());
			const auto &blsKeysEntry = BlsKeyIter.get();
			test::AssertEqualBlskeyData(values.BlsKeyEntry, blsKeysEntry);

        }
    }

    TEST(TEST_CLASS, ReplicatorOnboarding_Commit) {
        // Arrange:
        CacheValues values;
        values.ReplicatorEntry = CreateReplicatorEntry();
        values.BlsKeyEntry = CreateBlsKeyEntry();

        // Assert:
        RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, ReplicatorOnboarding_Rollback) {
        // Arrange:
        CacheValues values;
        values.ReplicatorEntry = CreateReplicatorEntry();
        values.BlsKeyEntry = CreateBlsKeyEntry();

        // Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values, Current_Height), catapult_runtime_error);
    }
}}