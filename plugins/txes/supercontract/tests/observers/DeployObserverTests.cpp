/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "plugins/txes/service/tests/test/ServiceTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS DeployObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(Deploy, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::SuperContractCacheFactory>;
		using Notification = model::DeployNotification<1>;

		const auto Current_Height = test::GenerateRandomValue<Height>();
		const auto Drive_Key = test::GenerateRandomByteArray<Key>();
		const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
		const auto Super_Contract_Owner_Key = test::GenerateRandomByteArray<Key>();
		const auto File_Hash = test::GenerateRandomByteArray<Hash256>();
		const auto Vm_Version = test::GenerateRandomValue<VmVersion>();

		auto CreateDriveEntry(std::initializer_list<Key> superContractKeys) {
			state::DriveEntry entry(Drive_Key);
			for (const auto& key : superContractKeys)
				entry.coowners().insert(key);

			return entry;
		}

		auto CreateSuperContractEntry() {
			state::SuperContractEntry entry(Super_Contract_Key);
			entry.setOwner(Super_Contract_Owner_Key);
			entry.setStart(Current_Height);
			entry.setVmVersion(Vm_Version);
			entry.setFileHash(File_Hash);
			entry.setMainDriveKey(Drive_Key);

			return entry;
		}

		auto CreateNotification() {
			return Notification(
				Super_Contract_Key,
				Super_Contract_Owner_Key,
				Drive_Key,
				File_Hash,
				Vm_Version);
		}
	}

	TEST(TEST_CLASS, Deploy_Commit) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Commit, Current_Height);
		auto notification = CreateNotification();
		auto pObserver = CreateDeployObserver();
		auto& superContractCache = context.cache().sub<cache::SuperContractCache>();
		auto& driveCache = context.cache().sub<cache::DriveCache>();

		// Populate cache.
		driveCache.insert(CreateDriveEntry({}));

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto superContractCacheIter = superContractCache.find(Super_Contract_Key);
		const auto& actualSuperContractEntry = superContractCacheIter.get();
		test::AssertEqualSuperContractData(CreateSuperContractEntry(), actualSuperContractEntry);

		auto driveCacheIter = driveCache.find(Drive_Key);
		const auto& actualDriveEntry = driveCacheIter.get();
		test::AssertEqualDriveData(CreateDriveEntry({Super_Contract_Key}), actualDriveEntry);
	}

	TEST(TEST_CLASS, Deploy_Rollback) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Rollback, Current_Height);
		auto notification = CreateNotification();
		auto pObserver = CreateDeployObserver();
		auto& superContractCache = context.cache().sub<cache::SuperContractCache>();
		auto& driveCache = context.cache().sub<cache::DriveCache>();

		// Populate cache.
		superContractCache.insert(CreateSuperContractEntry());
		driveCache.insert(CreateDriveEntry({Super_Contract_Key}));

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto superContractCacheIter = superContractCache.find(Super_Contract_Key);
		EXPECT_EQ(nullptr, superContractCacheIter.tryGet());

		auto driveCacheIter = driveCache.find(Drive_Key);
		const auto& actualDriveEntry = driveCacheIter.get();
		test::AssertEqualDriveData(CreateDriveEntry({}), actualDriveEntry);
	}
}}
