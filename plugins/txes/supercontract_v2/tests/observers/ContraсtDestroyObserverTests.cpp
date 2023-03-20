/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "plugins/txes/storage/src/observers/StorageExternalManagementObserverImpl.h"

namespace catapult { namespace observers {

#define TEST_CLASS ContractDestroyObserverTests

    const std::unique_ptr<observers::StorageExternalManagementObserver> Storage_External_Manager = std::make_unique<observers::StorageExternalManagementObserverImpl>();

	DEFINE_COMMON_OBSERVER_TESTS(ContractDestroy, Storage_External_Manager)

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::SuperContractCacheFactory>;
		using Notification = model::ContractDestroyNotification<1>;

        const auto Storage_Mosaic_Id = MosaicId(1324);
		const auto Current_Height = test::GenerateRandomValue<Height>();
        const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
        const auto Drive_Key = test::GenerateRandomByteArray<Key>();
		const auto Assignee = test::GenerateRandomByteArray<Key>();
		const auto Execution_Payment_Key = test::GenerateRandomByteArray<Key>();

		auto CreateDriveContractEntry() {
			auto driveEntry = test::CreateDriveContractEntry(Drive_Key, Super_Contract_Key);

			return driveEntry;
		}

		auto CreateSuperContractEntry() {
			state::SuperContractEntry entry(Super_Contract_Key);
			entry.setDriveKey(Drive_Key);
            entry.setAssignee(Assignee);
            entry.setExecutionPaymentKey(Execution_Payment_Key);

			return entry;
		}

		auto CreateNotification() {
			return Notification(Super_Contract_Key);
		}

        struct CacheValues {
        public:
            CacheValues() :
            InitialScEntry(Key()),
            InitialDriveContractEntry(Key()) {}

        public:
            state::SuperContractEntry InitialScEntry;
            state::DriveContractEntry InitialDriveContractEntry;
            Amount InitialContractExecutionBalances;
            Amount ExpectedContractExecutionBalances;
            Amount ExpectedAssignee;
        };

        void RunTest(NotifyMode mode, const CacheValues& values) {
            // Arrange:
            ObserverTestContext context(mode, Current_Height);
            auto notification = CreateNotification();
            auto pObserver = CreateContractDestroyObserver(Storage_External_Manager);
            auto& superContractCache = context.cache().sub<cache::SuperContractCache>();
            auto& driveContractCache = context.cache().sub<cache::DriveContractCache>();
            auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();
            auto& accountCache = context.cache().sub<cache::AccountStateCache>();

            // Populate cache.
            state::BcDriveEntry bcDrive(values.InitialScEntry.driveKey());
            bcDriveCache.insert(bcDrive);
            driveContractCache.insert(values.InitialDriveContractEntry);
            superContractCache.insert(values.InitialScEntry);
            accountCache.addAccount(values.InitialScEntry.key(), Height(1));
            accountCache.addAccount(values.InitialScEntry.driveKey(), Height(1));
            accountCache.addAccount(values.InitialScEntry.assignee(), Height(1));
            accountCache.addAccount(values.InitialScEntry.executionPaymentKey(), Height(1));

            auto contractExecutionAccountEntry = accountCache.find(values.InitialScEntry.executionPaymentKey()).get();
            contractExecutionAccountEntry.Balances.credit(Storage_Mosaic_Id,values.InitialContractExecutionBalances);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto superContractCacheIter = superContractCache.find(values.InitialScEntry.key());
            const auto actualScEntry = superContractCacheIter.tryGet();
            EXPECT_TRUE(!actualScEntry);

            auto driveCacheIter = driveContractCache.find(values.InitialScEntry.driveKey());
            const auto actualDriveContractEntry = driveCacheIter.tryGet();
            EXPECT_TRUE(!actualDriveContractEntry);

            auto bcDriveIter = bcDriveCache.find(values.InitialScEntry.driveKey());
            const auto bcDriveEntry = bcDriveIter.tryGet();
            EXPECT_EQ(bcDriveEntry->ownerManagement(), state::OwnerManagement::ALLOWED);

            const auto executionPaymentAccountEntry = accountCache.find(values.InitialScEntry.executionPaymentKey()).get();
            EXPECT_EQ(values.ExpectedContractExecutionBalances, executionPaymentAccountEntry.Balances.get(Storage_Mosaic_Id));

            const auto assigneeAccountEntry = accountCache.find(values.InitialScEntry.assignee()).get();
            EXPECT_EQ(values.ExpectedAssignee, assigneeAccountEntry.Balances.get(Storage_Mosaic_Id));
        }
	}

	TEST(TEST_CLASS, Commit) {
        // Arrange
        CacheValues values;
        values.InitialScEntry = CreateSuperContractEntry();
        values.InitialDriveContractEntry = CreateDriveContractEntry();
        values.InitialContractExecutionBalances = Amount(111);
        values.ExpectedAssignee = values.InitialContractExecutionBalances;
        values.ExpectedContractExecutionBalances = Amount(0);

		// Assert
		RunTest(NotifyMode::Commit, values);
	}

	TEST(TEST_CLASS, Rollback) {
        // Arrange
        CacheValues values;
        values.InitialScEntry = CreateSuperContractEntry();
        values.InitialDriveContractEntry = CreateDriveContractEntry();

		// Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values), catapult_runtime_error);
	}
}}
