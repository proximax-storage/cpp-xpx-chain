/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS FilesDepositObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(FilesDeposit, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::DriveCacheFactory>;
		using Notification = model::FilesDepositNotification<1>;

		constexpr Height Current_Height(10);
		const Key Replicator_Key = test::GenerateRandomByteArray<Key>();

		void AddFiles(state::DriveEntry& entry, uint8_t numFiles) {
			for (auto i = 0u; i < numFiles; ++i) {
				auto fileHash = test::GenerateRandomByteArray<Hash256>();
				uint64_t fileSize = (i + 1) * 100;
				entry.files().emplace(fileHash, state::FileInfo{ fileSize });
				for (auto& replicator : entry.replicators())
					replicator.second.ActiveFilesWithoutDeposit.insert(fileHash);
			}
		}

		state::DriveEntry CreateInitialDriveEntry(
				uint8_t numFiles,
				uint8_t numReplicators) {
			state::DriveEntry entry(test::GenerateRandomByteArray<Key>());

			entry.replicators().emplace(Replicator_Key, state::ReplicatorInfo());
			for (auto i = 0u; i < numReplicators; ++i) {
				entry.replicators().emplace(test::GenerateRandomByteArray<Key>(), state::ReplicatorInfo());
			}

			AddFiles(entry, numFiles);

			return entry;
		}

		state::DriveEntry CreateExpectedDriveEntry(
				const state::DriveEntry& initialEntry,
				uint8_t numFiles,
				std::vector<model::File>& depositedFiles) {
			state::DriveEntry entry(initialEntry);
			auto& replicator = entry.replicators().at(Replicator_Key);
			auto iter = entry.files().begin();
			depositedFiles.reserve(numFiles);
			for (auto i = 0u; i < numFiles && iter != entry.files().end(); ++i, ++iter) {
				replicator.ActiveFilesWithoutDeposit.erase(iter->first);
				depositedFiles.push_back(model::File{ iter->first });
			}

			return entry;
		}

		struct CacheValues {
		public:
			CacheValues() : InitialDriveEntry(Key()), ExpectedDriveEntry(Key())
			{}

		public:
			state::DriveEntry InitialDriveEntry;
			state::DriveEntry ExpectedDriveEntry;
			std::vector<model::File> DepositedFiles;
		};

		void RunTest(NotifyMode mode, const CacheValues& values) {
			// Arrange:
			ObserverTestContext context(mode, Current_Height);
			Notification notification(
				values.InitialDriveEntry.key(),
				Replicator_Key,
				values.DepositedFiles.size(),
				values.DepositedFiles.data());
			auto pObserver = CreateFilesDepositObserver();
			auto& driveCache = context.cache().sub<cache::DriveCache>();

			// Populate drive cache.
			driveCache.insert(values.InitialDriveEntry);

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			auto iter = driveCache.find(values.ExpectedDriveEntry.key());
			const auto& actualEntry = iter.get();
			test::AssertEqualDriveData(values.ExpectedDriveEntry, actualEntry);
		}
	}

	TEST(TEST_CLASS, FilesDeposit_Commit) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(5, 10);
		values.ExpectedDriveEntry = CreateExpectedDriveEntry(values.InitialDriveEntry, 4, values.DepositedFiles);

		// Assert:
		RunTest(NotifyMode::Commit, values);
	}

	TEST(TEST_CLASS, FilesDeposit_Rollback) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(5, 10);
		values.InitialDriveEntry = CreateExpectedDriveEntry(values.ExpectedDriveEntry, 4, values.DepositedFiles);

		// Assert:
		RunTest(NotifyMode::Rollback, values);
	}
}}
