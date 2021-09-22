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
#include "catapult/model/Address.h"

namespace catapult { namespace observers {

#define TEST_CLASS ReplicatorOffboardingObserverTests

    DEFINE_COMMON_OBSERVER_TESTS(ReplicatorOffboarding,)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::ReplicatorCacheFactory>;
        using Notification = model::ReplicatorOffboardingNotification<1>;

        const Key Replicator_Key = test::GenerateRandomByteArray<Key>();
        const Key Drive_Key = test::GenerateRandomByteArray<Key>();
        const Key Owner_key = test::GenerateRandomByteArray<Key>();
        constexpr auto Current_Height = Height(25);
        uint64_t Storage_Deposit = 100;
        uint64_t Streaming_Deposit = 200;

        state::BcDriveEntry CreateBcDriveEntry() {
            state::BcDriveEntry entry(Drive_Key);
            entry.setOwner(Owner_key);
            entry.setUsedSize(25);
            entry.confirmedUsedSizes().insert({Replicator_Key, 25});

            return entry;
        }

        state::ReplicatorEntry CreateReplicatorEntry() {
            state::ReplicatorEntry entry(Replicator_Key);
            entry.setCapacity(Amount(Storage_Deposit));
            entry.drives().emplace(Drive_Key, state::DriveInfo{});
            return entry;
        }

        state::AccountState CreateAccount(catapult::MosaicId Storage_Mosaic_Id, catapult::MosaicId Streaming_Mosaic_Id) {
		    auto address = model::PublicKeyToAddress(Replicator_Key, model::NetworkIdentifier::Mijin_Test);
		    state::AccountState account(address, Current_Height);
		    account.PublicKey = Replicator_Key;
		    account.PublicKeyHeight = Current_Height;
            account.Balances.credit(Storage_Mosaic_Id, Amount(Storage_Deposit));
            account.Balances.credit(Streaming_Mosaic_Id, Amount(Streaming_Deposit));
		    return account;
	    }

        struct ReplicatorValues {
            public:
                explicit ReplicatorValues()
                    : PublicKey(test::GenerateRandomByteArray<Key>())
                {}

            public:
                Key PublicKey;
        };

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

        void RunTest(NotifyMode mode, ReplicatorValues values, const Height& Current_Height) {
            // Arrange:
            ObserverTestContext context(mode, Current_Height);
            Notification notification(Replicator_Key);
            auto pObserver = CreateReplicatorOffboardingObserver();
        	auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();
            auto& accountCache = context.cache().sub<cache::AccountStateCache>();
            auto& driveCache = context.cache().sub<cache::BcDriveCache>();
            auto& Currency_Mosaic_Id = context.observerContext().Config.Immutable.CurrencyMosaicId;
            auto& Storage_Mosaic_Id = context.observerContext().Config.Immutable.CurrencyMosaicId;
            auto& Streaming_Mosaic_Id = context.observerContext().Config.Immutable.CurrencyMosaicId;

            //Populate cache
            replicatorCache.insert(CreateReplicatorEntry());
            driveCache.insert(CreateBcDriveEntry());
            accountCache.addAccount(CreateAccount(Storage_Mosaic_Id, Streaming_Mosaic_Id));

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
			EXPECT_FALSE(replicatorCache.find(Replicator_Key).tryGet());
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
}}