/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/StorageTestUtils.h"
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

		Key Replicator_Key = test::GenerateRandomByteArray<Key>();
		Key Drive_Key1 = test::GenerateRandomByteArray<Key>();
		Key Drive_Key2 = test::GenerateRandomByteArray<Key>();
        constexpr auto Current_Height = Height(25);
		constexpr auto Capacity = Amount(30);
		constexpr uint64_t Drive1_Size = 100;
		constexpr uint64_t Drive1_Used_Size = 40;
		constexpr uint64_t Drive2_Size = 200;
		constexpr uint64_t Drive2_Confirmed_Used_Size = 50;
		constexpr uint64_t Drive2_Used_Size = 60;
		constexpr uint64_t Expected_Storage_Deposit_Return = Capacity.unwrap() + Drive1_Size + Drive2_Size;
		constexpr uint64_t Expected_Streaming_Deposit_Return = 2 * (Expected_Storage_Deposit_Return - Drive1_Used_Size - Drive2_Confirmed_Used_Size);
		constexpr auto Expected_Replicator_Balance = Amount(Expected_Storage_Deposit_Return + Expected_Streaming_Deposit_Return);
		constexpr auto Currency_Mosaic_Id = MosaicId(1234);

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.CurrencyMosaicId = Currency_Mosaic_Id;
			return config.ToConst();
		}

        std::vector<state::BcDriveEntry> CreateDriveEntries() {
			std::vector<state::BcDriveEntry> ret;
            state::BcDriveEntry entry1(Drive_Key1);
            entry1.setSize(Drive1_Size);
            entry1.setUsedSize(Drive1_Used_Size);
			ret.push_back(entry1);

            state::BcDriveEntry entry2(Drive_Key2);
            entry2.setSize(Drive2_Size);
            entry2.setUsedSize(Drive2_Used_Size);
            entry2.confirmedUsedSizes().insert({ Replicator_Key, Drive2_Confirmed_Used_Size });
			ret.push_back(entry2);

            return ret;
        }

        state::ReplicatorEntry CreateReplicatorEntry() {
            state::ReplicatorEntry entry(Replicator_Key);
            entry.setCapacity(Capacity);
            entry.drives().emplace(Drive_Key1, state::DriveInfo{});
            entry.drives().emplace(Drive_Key2, state::DriveInfo{});
            return entry;
        }

        state::AccountState CreateAccount() {
		    auto address = model::PublicKeyToAddress(Replicator_Key, model::NetworkIdentifier::Mijin_Test);
		    state::AccountState account(address, Current_Height);
		    account.PublicKey = Replicator_Key;
		    account.PublicKeyHeight = Current_Height;
		    return account;
	    }

        void RunTest(NotifyMode mode) {
            // Arrange:
            ObserverTestContext context(mode, Current_Height, CreateConfig());
            Notification notification(Replicator_Key);
            auto pObserver = CreateReplicatorOffboardingObserver();
        	auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();
            auto& accountCache = context.cache().sub<cache::AccountStateCache>();
            auto& driveCache = context.cache().sub<cache::BcDriveCache>();

            //Populate cache
            replicatorCache.insert(CreateReplicatorEntry());
			auto driveEntries = CreateDriveEntries();
			for (const auto& driveEntry : driveEntries)
            	driveCache.insert(driveEntry);
            accountCache.addAccount(CreateAccount());

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
			EXPECT_FALSE(replicatorCache.find(Replicator_Key).tryGet());
            test::AssertBalances(context.cache(), Replicator_Key, { model::Mosaic{ Currency_Mosaic_Id, Expected_Replicator_Balance } });
        }
    }

    TEST(TEST_CLASS, ReplicatorOffboarding_Commit) {
        // Assert:
        RunTest(NotifyMode::Commit);
    }

    TEST(TEST_CLASS, DriveClosure_Rollback) {
        // Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback), catapult_runtime_error);
    }
}}