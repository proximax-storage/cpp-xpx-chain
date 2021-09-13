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
#include "tests/test/cache/BalanceTransferTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS ReplicatorOffboardingObserverTests

    DEFINE_COMMON_OBSERVER_TESTS(ReplicatorOffboarding,)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::ReplicatorCacheFactory>;
        using Notification = model::ReplicatorOffboardingNotification<1>;

        const Key Drive_Key = test::GenerateRandomByteArray<Key>();
        const Key Owner = test::GenerateRandomByteArray<Key>();
        constexpr auto Current_Height = Height(25);
        constexpr auto Currency_Mosaic_Id = MosaicId(1000);
        constexpr auto Storage_Mosaic_Id = MosaicId(1001);
        constexpr auto Streaming_Mosaic_Id = MosaicId(1002);
        uint64_t storageDeposit = 100;
        uint64_t streamingDeposit = 200;

        struct ReplicatorValues {
            public:
                explicit ReplicatorValues()
                    : PublicKey(test::GenerateRandomByteArray<Key>())
                {}

            public:
                Key PublicKey;
        };

        state::BcDriveEntry CreateBcDriveEntry(const ReplicatorValues& values) {
            state::BcDriveEntry entry(Drive_Key);
            entry.setOwner(Owner);
			entry.setSize(50);
            entry.setUsedSize(25);
            entry.confirmedUsedSizes().insert({values.PublicKey, 25});

            return entry;
        }

        state::ReplicatorEntry CreateReplicatorEntry(const ReplicatorValues& values) {
            state::ReplicatorEntry entry(values.PublicKey);
            entry.setCapacity(Amount(storageDeposit));
            entry.drives().emplace(Drive_Key, state::DriveInfo{});
            return entry;
        }

        static test::BalanceTransfers GetInitialReplicatorDepositUnits(MosaicId depositUnit, uint64_t totalUnit) {
            return { { depositUnit, Amount(totalUnit) } };
        }
        
        static test::BalanceTransfers GetFinalReplicatorStorage(MosaicId mosaicId) {
            //total storage deposit = 100 (capacity + usedSize)
            return { { mosaicId, Amount(0) } };
        }
        
        static test::BalanceTransfers GetFinalReplicatorStreaming(MosaicId mosaicId) {
            //total streaming deposit = 50   (2*50)-(2min(25, 25))   
            return { { mosaicId, Amount(200 - 50) } };
        }
        
        static test::BalanceTransfers GetFinalReplicatorCurrency(MosaicId mosaicId) {
            //storage + streaming = 150
            return { { mosaicId, Amount(100) } };
        }

        void RunTest(NotifyMode mode, const ReplicatorValues& values, const Height& currentHeight) {
            // Arrange:
            ObserverTestContext context(mode, currentHeight);
            Notification notification(values.PublicKey);
            auto pObserver = CreateReplicatorOffboardingObserver();
        	auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();
            auto& accountCache = context.cache().sub<cache::AccountStateCache>();
            auto& driveCache = context.cache().sub<cache::BcDriveCache>();

            //Set initial storage and streaming unit in the replicator
            test::SetCacheBalances(context.cache(), values.PublicKey, GetInitialReplicatorDepositUnits(Storage_Mosaic_Id, storageDeposit));
            test::SetCacheBalances(context.cache(), values.PublicKey, GetInitialReplicatorDepositUnits(Streaming_Mosaic_Id, streamingDeposit));

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
     		auto replicatorIter = replicatorCache.find(values.PublicKey);
			const auto &actualEntry = replicatorIter.get();
			test::AssertEqualReplicatorData(CreateReplicatorEntry(values), actualEntry);
            test::AssertBalances(context.cache(), values.PublicKey, GetFinalReplicatorCurrency(Currency_Mosaic_Id));
            test::AssertBalances(context.cache(), values.PublicKey, GetFinalReplicatorStorage(Storage_Mosaic_Id));
            test::AssertBalances(context.cache(), values.PublicKey, GetFinalReplicatorStreaming(Streaming_Mosaic_Id));
        }
    }

    TEST(TEST_CLASS, ReplicatorOffboarding_Commit) {
        // Arrange:
        ReplicatorValues values;

        // Assert:
        RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, ReplicatorOffboarding_Rollback) {
        // Arrange:
        ReplicatorValues values;

        // Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values, Current_Height), catapult_runtime_error);
    }
}}