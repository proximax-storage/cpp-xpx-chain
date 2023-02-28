/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "plugins/txes/storage/src/state/DriveStateBrowserImpl.h"
#include "plugins/txes/supercontract/src/observers/Observers.h"

namespace catapult { namespace observers {

#define TEST_CLASS DeployContractObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(DeployContract, std::make_unique<state::DriveStateBrowserImpl>())

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
		const auto Base_Modification_Id = test::GenerateRandomByteArray<Hash256>();

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

			return entry;
		}

		auto CreateNotification() {
			return Notification(
					Super_Contract_Key,
					Drive_Key,
					Super_Contract_Owner_Key,
					Super_Contract_Owner_Key,
					File_Name,
					Function_Name,
					Execution_Call_Payment,
					Download_Call_Payment);
		}
	}

	void RunTest(NotifyMode mode, const Height& currentHeight) {
		// Arrange:
		std::unique_ptr<state::DriveStateBrowser> browser = std::make_unique<state::DriveStateBrowserImpl>();
		ObserverTestContext context(mode, Current_Height);
		auto notification = CreateNotification();
		auto pObserver = CreateDeployContractObserver(browser);
		auto& superContractCache = context.cache().sub<cache::SuperContractCache>();
		auto& driveCache = context.cache().sub<cache::DriveContractCache>();

		// Populate cache.
		auto scEntry = CreateSuperContractEntry();
		auto driveContractEntry = CreateDriveContractEntry();
		if (mode == NotifyMode::Rollback) {
			superContractCache.insert(scEntry);
			driveCache.insert(driveContractEntry);
		}

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto superContractCacheIter = superContractCache.find(scEntry.key());
		EXPECT_EQ(nullptr, superContractCacheIter.tryGet());

		auto driveCacheIter = driveCache.find(scEntry.driveKey());
		const auto& actualDriveEntry = driveCacheIter.get();
		test::AssertEqualDriveContract(driveContractEntry, actualDriveEntry);
	}

	TEST(TEST_CLASS, Deploy_Commit) {
		// Assert
		RunTest(NotifyMode::Commit, Current_Height);
	}

	TEST(TEST_CLASS, Deploy_Rollback) {
		// Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, Current_Height), catapult_runtime_error);
	}
}}
