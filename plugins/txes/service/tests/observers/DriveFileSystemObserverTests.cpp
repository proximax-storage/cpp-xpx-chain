/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS DriveFileSystemObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(DriveFileSystem, MosaicId())

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::DriveCacheFactory>;
		using Notification = model::DriveFileSystemNotification<1>;

		constexpr auto Streaming_Mosaic_Id = MosaicId(1234u);
		constexpr Height Current_Height(10);
		const Hash256 Root_Hash = test::GenerateRandomByteArray<Hash256>();
		const Hash256 Xor_Root_Hash = test::GenerateRandomByteArray<Hash256>();

		void AddFiles(
				state::DriveEntry& entry,
				uint8_t numFiles,
				std::vector<model::AddAction>* pAddActions) {
			if (!!pAddActions)
				pAddActions->reserve(numFiles);

			for (auto i = 0u; i < numFiles; ++i) {
				auto fileHash = test::GenerateRandomByteArray<Hash256>();
				uint64_t fileSize = (i + 1) * 100;
				entry.files().emplace(fileHash, state::FileInfo{ fileSize });
				entry.increaseOccupiedSpace(fileSize);
				if (!!pAddActions)
					pAddActions->push_back(model::AddAction{ { fileHash }, fileSize });
				for (auto& replicator : entry.replicators())
					replicator.second.ActiveFilesWithoutDeposit.insert(fileHash);
			}
		}

		state::DriveEntry CreateInitialDriveEntry(
				uint8_t numFiles,
				uint8_t numReplicators,
				std::map<Key, state::AccountState>& accounts) {
			state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
			entry.setRootHash(Root_Hash ^ Xor_Root_Hash);

			for (auto i = 0u; i < numReplicators; ++i) {
				auto key = test::GenerateRandomByteArray<Key>();
				entry.replicators().emplace(key, state::ReplicatorInfo());
				accounts.emplace(key, test::CreateAccount(key));
			}

			AddFiles(entry, numFiles, nullptr);

			return entry;
		}

		state::DriveEntry CreateExpectedDriveEntry(
				state::DriveEntry& initialEntry,
				uint8_t numFiles,
				uint8_t numRemovedFiles,
				std::vector<model::AddAction>& addActions,
				std::vector<model::RemoveAction>& removeActions,
				std::map<Key, state::AccountState>& accounts) {
			state::DriveEntry entry(initialEntry);
			entry.setRootHash(Root_Hash);

			removeActions.reserve(numRemovedFiles);
			auto& files = entry.files();
			auto fileIter = files.begin();
			for (auto i = 0u; i < numRemovedFiles && fileIter != files.end(); ++i) {
				removeActions.push_back(model::RemoveAction{ { {fileIter->first}, fileIter->second.Size } });
				auto r = 0u;
				for (auto& replicatorPair : entry.replicators()) {
					replicatorPair.second.ActiveFilesWithoutDeposit.erase(fileIter->first);
					if (r++ % 2) {
						replicatorPair.second.AddInactiveUndepositedFile(fileIter->first, Current_Height);
					} else {
						initialEntry.replicators().at(replicatorPair.first).ActiveFilesWithoutDeposit.erase(fileIter->first);
						accounts.at(replicatorPair.first).Balances.credit(Streaming_Mosaic_Id, Amount(fileIter->second.Size), Current_Height);
					}
				}
				entry.decreaseOccupiedSpace(fileIter->second.Size);
				fileIter = files.erase(fileIter);
			}

			AddFiles(entry, numFiles, &addActions);

			return entry;
		}

		struct CacheValues {
		public:
			CacheValues() : InitialDriveEntry(Key()), ExpectedDriveEntry(Key())
			{}

		public:
			state::DriveEntry InitialDriveEntry;
			state::DriveEntry ExpectedDriveEntry;
			std::vector<model::AddAction> AddActions;
			std::vector<model::RemoveAction> RemoveActions;
			std::map<Key, state::AccountState> InitialAccounts;
			std::map<Key, state::AccountState> ExpectedAccounts;
		};

		void RunTest(NotifyMode mode, const CacheValues& values) {
			// Arrange:
			ObserverTestContext context(mode, Current_Height);
			Notification notification(
				values.InitialDriveEntry.key(),
				test::GenerateRandomByteArray<Key>(),
				Root_Hash,
				Xor_Root_Hash,
				values.AddActions.size(),
				values.AddActions.data(),
				values.RemoveActions.size(),
				values.RemoveActions.data());
			auto pObserver = CreateDriveFileSystemObserver(Streaming_Mosaic_Id);
			auto& driveCache = context.cache().sub<cache::DriveCache>();
			auto& accountCache = context.cache().sub<cache::AccountStateCache>();

			// Populate drive cache.
			driveCache.insert(values.InitialDriveEntry);

			// Populate account cache.
			for (const auto& initialAccount : values.InitialAccounts) {
				accountCache.addAccount(initialAccount.second);
			}

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			auto driveIter = driveCache.find(values.ExpectedDriveEntry.key());
			const auto& actualEntry = driveIter.get();
			test::AssertEqualDriveData(values.ExpectedDriveEntry, actualEntry);

			for (const auto& expectedAccount : values.ExpectedAccounts) {
				auto accountIter = accountCache.find(expectedAccount.second.Address);
				const auto& actualAccount = accountIter.get();
				test::AssertAccount(expectedAccount.second, actualAccount);
			}
		}
	}

	TEST(TEST_CLASS, DriveFileSystem_Commit) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(5, 10, values.InitialAccounts);
		values.ExpectedAccounts = values.InitialAccounts;
		values.ExpectedDriveEntry = CreateExpectedDriveEntry(values.InitialDriveEntry, 4, 3, values.AddActions, values.RemoveActions, values.ExpectedAccounts);

		// Assert:
		RunTest(NotifyMode::Commit, values);
	}

	TEST(TEST_CLASS, DriveFileSystem_Rollback) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(5, 10, values.InitialAccounts);
		values.ExpectedAccounts = values.InitialAccounts;
		values.InitialDriveEntry = CreateExpectedDriveEntry(values.ExpectedDriveEntry, 4, 3, values.AddActions, values.RemoveActions, values.InitialAccounts);

		// Assert:
		RunTest(NotifyMode::Rollback, values);
	}
}}
