/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/DriveCache.h"
#include "src/validators/Validators.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS DriveFileSystemValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(DriveFileSystem, )

	namespace {
		using Notification = model::DriveFileSystemNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::DriveEntry& entry,
				const Key& signer,
				const Hash256& rootHash,
				const Hash256& xorRootHash,
				const std::vector<model::AddAction>& addActions = {},
				const std::vector<model::RemoveAction>& removeActions = {}) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::DriveCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& driveCacheDelta = delta.sub<cache::DriveCache>();
				driveCacheDelta.insert(entry);
				cache.commit(currentHeight);
			}
			Notification notification(entry.key(), signer, rootHash, xorRootHash, addActions.size(), addActions.data(), removeActions.size(), removeActions.data());
			auto pValidator = CreateDriveFileSystemValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenOperationIsNotPermitted) {
		// Assert:
		AssertValidationResult(
			Failure_Service_Operation_Is_Not_Permitted,
			state::DriveEntry(test::GenerateRandomByteArray<Key>()),
			test::GenerateRandomByteArray<Key>(),
			test::GenerateRandomByteArray<Hash256>(),
			test::GenerateRandomByteArray<Hash256>());
	}

	TEST(TEST_CLASS, FailureWhenRootHashNotChanged) {
		// Arrange:
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setOwner(test::GenerateRandomByteArray<Key>());

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Root_No_Changes,
			entry,
			entry.owner(),
			test::GenerateRandomByteArray<Hash256>(),
			Hash256());
	}

	TEST(TEST_CLASS, FailureWhenRootHashNotCompatible) {
		// Arrange:
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setOwner(test::GenerateRandomByteArray<Key>());

		// Assert:
		AssertValidationResult(
			Failure_Service_Root_Hash_Is_Not_Equal,
			entry,
			entry.owner(),
			test::GenerateRandomByteArray<Hash256>(),
			test::GenerateRandomByteArray<Hash256>());
	}

	TEST(TEST_CLASS, FailureWhenFileExists) {
		// Arrange:
		auto rootHash = test::GenerateRandomByteArray<Hash256>();
		auto xorRootHash = test::GenerateRandomByteArray<Hash256>();
		auto fileHash = test::GenerateRandomByteArray<Hash256>();
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setOwner(test::GenerateRandomByteArray<Key>());
		entry.setRootHash(xorRootHash ^ rootHash);
		entry.files().emplace(fileHash, state::FileInfo{ 10 });

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Hash_Redundant,
			entry,
			entry.owner(),
			rootHash,
			xorRootHash,
			{ { {fileHash}, 20 } });
	}

	TEST(TEST_CLASS, FailureWhenFileHashRedundant) {
		// Arrange:
		auto rootHash = test::GenerateRandomByteArray<Hash256>();
		auto xorRootHash = test::GenerateRandomByteArray<Hash256>();
		auto fileHash = test::GenerateRandomByteArray<Hash256>();
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setOwner(test::GenerateRandomByteArray<Key>());
		entry.setRootHash(xorRootHash ^ rootHash);

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Hash_Redundant,
			entry,
			entry.owner(),
			rootHash,
			xorRootHash,
			{ { {fileHash}, 20 }, { {fileHash}, 40 } });
	}

	TEST(TEST_CLASS, FailureWhenRemovingNotExistingFile) {
		// Arrange:
		auto rootHash = test::GenerateRandomByteArray<Hash256>();
		auto xorRootHash = test::GenerateRandomByteArray<Hash256>();
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setOwner(test::GenerateRandomByteArray<Key>());
		entry.setRootHash(xorRootHash ^ rootHash);

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Doesnt_Exist,
			entry,
			entry.owner(),
			rootHash,
			xorRootHash,
			{ { { test::GenerateRandomByteArray<Hash256>() }, 20 } },
			{ { { { test::GenerateRandomByteArray<Hash256>() }, 40 } } });
	}

	TEST(TEST_CLASS, FailureWhenRemovingFileWrongSize) {
		// Arrange:
		auto rootHash = test::GenerateRandomByteArray<Hash256>();
		auto xorRootHash = test::GenerateRandomByteArray<Hash256>();
		auto fileHash = test::GenerateRandomByteArray<Hash256>();
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setOwner(test::GenerateRandomByteArray<Key>());
		entry.setRootHash(xorRootHash ^ rootHash);
		entry.files().emplace(fileHash, state::FileInfo{ 10 });

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Size_Invalid,
			entry,
			entry.owner(),
			rootHash,
			xorRootHash,
			{ { { test::GenerateRandomByteArray<Hash256>() }, 20 } },
			{ { { { fileHash }, 40 } } });
	}

	TEST(TEST_CLASS, FailureWhenRemovingFileHashRedundant) {
		// Arrange:
		auto rootHash = test::GenerateRandomByteArray<Hash256>();
		auto xorRootHash = test::GenerateRandomByteArray<Hash256>();
		auto fileHash = test::GenerateRandomByteArray<Hash256>();
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setOwner(test::GenerateRandomByteArray<Key>());
		entry.setRootHash(xorRootHash ^ rootHash);
		entry.files().emplace(fileHash, state::FileInfo{ 40 });

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Hash_Redundant,
			entry,
			entry.owner(),
			rootHash,
			xorRootHash,
			{ { { test::GenerateRandomByteArray<Hash256>() }, 20 } },
			{ { { { fileHash }, 40 } }, { { { fileHash }, 40 } } });
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		auto rootHash = test::GenerateRandomByteArray<Hash256>();
		auto xorRootHash = test::GenerateRandomByteArray<Hash256>();
		auto fileHash = test::GenerateRandomByteArray<Hash256>();
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setOwner(test::GenerateRandomByteArray<Key>());
		entry.setRootHash(xorRootHash ^ rootHash);
		entry.files().emplace(fileHash, state::FileInfo{ 40 });

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			entry,
			entry.owner(),
			rootHash,
			xorRootHash,
			{ { { test::GenerateRandomByteArray<Hash256>() }, 20 } },
			{ { { { fileHash }, 40 } } });
	}
}}
