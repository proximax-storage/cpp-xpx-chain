/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS DeployContractObserverTests

	const std::unique_ptr<state::DriveStateBrowser> Drive_Browser = std::make_unique<test::DriveStateBrowserImpl>();

	DEFINE_COMMON_OBSERVER_TESTS(DeployContract, Drive_Browser)

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::SuperContractCacheFactory>;
		using Notification = model::DeploySupercontractNotification<1>;

		const auto Current_Height = test::GenerateRandomValue<Height>();
		const auto Drive_Key = test::GenerateRandomByteArray<Key>();
		const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
		const auto Super_Contract_Owner_Key = test::GenerateRandomByteArray<Key>();
		const auto File_Name = test::GenerateRandomString(10);
		const auto Function_Name = test::GenerateRandomString(10);
		const auto Execution_Call_Payment = Amount(10);
		const auto Download_Call_Payment = Amount(10);
		const auto Base_Modification_Id = Hash256();

		auto CreateDriveContractEntry() {
			auto driveEntry = test::CreateDriveContractEntry(Drive_Key, Super_Contract_Key);

			return driveEntry;
		}

		auto CreateSuperContractEntry() {
			state::SuperContractEntry entry(Super_Contract_Key);
			entry.setDriveKey(Drive_Key);
			entry.setAssignee(Super_Contract_Owner_Key);
			entry.setExecutionPaymentKey(Super_Contract_Owner_Key);
			entry.setDeploymentBaseModificationId(Base_Modification_Id);

            auto& automaticExecutionsInfo = entry.automaticExecutionsInfo();
            automaticExecutionsInfo.AutomaticExecutionFileName = File_Name;
            automaticExecutionsInfo.AutomaticExecutionsFunctionName = Function_Name;
            automaticExecutionsInfo.AutomaticExecutionCallPayment = Execution_Call_Payment;
            automaticExecutionsInfo.AutomaticDownloadCallPayment = Download_Call_Payment;

			return entry;
		}

		auto CreateNotification() {
			return Notification(
					Super_Contract_Owner_Key,
					Super_Contract_Key,
					Drive_Key,
					Super_Contract_Owner_Key,
					Super_Contract_Owner_Key,
					File_Name,
					Function_Name,
					Execution_Call_Payment,
					Download_Call_Payment);
		}

        struct CacheValues {
        public:
            CacheValues() : ScEntry(Key()), DriveContractEntry(Key()) {}

        public:
            state::SuperContractEntry ScEntry;
            state::DriveContractEntry DriveContractEntry;
        };

        void RunTest(NotifyMode mode, const CacheValues& values) {
            // Arrange:
            ObserverTestContext context(mode, Current_Height);
            auto notification = CreateNotification();
            auto pObserver = CreateDeployContractObserver(Drive_Browser);
            auto& superContractCache = context.cache().sub<cache::SuperContractCache>();
            auto& driveCache = context.cache().sub<cache::DriveContractCache>();

            // Populate cache.
            if (mode == NotifyMode::Rollback) {
                superContractCache.insert(values.ScEntry);
                driveCache.insert(values.DriveContractEntry);
            }

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto superContractCacheIter = superContractCache.find(values.ScEntry.key());
            const auto& actualScEntry = superContractCacheIter.get();
            test::AssertEqualSuperContractData(values.ScEntry, actualScEntry);

            auto driveCacheIter = driveCache.find(values.ScEntry.driveKey());
            const auto& actualDriveContractEntry = driveCacheIter.get();
            test::AssertEqualDriveContract(values.DriveContractEntry, actualDriveContractEntry);
        }
	}

	TEST(TEST_CLASS, Deploy_Commit) {
        // Arrange
        CacheValues values;
        values.ScEntry = CreateSuperContractEntry();
        values.DriveContractEntry = CreateDriveContractEntry();

		// Assert
		RunTest(NotifyMode::Commit, values);
	}

	TEST(TEST_CLASS, Deploy_Rollback) {
        // Arrange
        CacheValues values;
        values.ScEntry = CreateSuperContractEntry();
        values.DriveContractEntry = CreateDriveContractEntry();

		// Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values), catapult_runtime_error);
	}
}}
