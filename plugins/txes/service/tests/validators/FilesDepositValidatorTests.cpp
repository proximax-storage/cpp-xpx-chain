/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS FilesDepositValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(FilesDeposit, )

	namespace {
		using Notification = model::FilesDepositNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::DriveEntry& entry,
				const Key& replicatorKey,
				const std::vector<model::File>& files = {}) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::DriveCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& driveCacheDelta = delta.sub<cache::DriveCache>();
				driveCacheDelta.insert(entry);
				cache.commit(currentHeight);
			}
			Notification notification(entry.key(), replicatorKey, files.size(), files.data());
			auto pValidator = CreateFilesDepositValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenReplicatorNotRegistered) {
		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Replicator_Not_Registered,
			state::DriveEntry(test::GenerateRandomByteArray<Key>()),
			test::GenerateRandomByteArray<Key>());
	}

	TEST(TEST_CLASS, FailureWhenFileDoesntExist) {
		// Arrange:
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		auto replicatorKey = test::GenerateRandomByteArray<Key>();
		state::ReplicatorInfo replicatorInfo{ Height(1), Height(0), {}, {} };
		entry.replicators().emplace(replicatorKey, replicatorInfo);

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Doesnt_Exist,
			entry,
			replicatorKey,
			{ { test::GenerateRandomByteArray<Hash256>() } });
	}

	TEST(TEST_CLASS, FailureWhenFileHashRedundant) {
		// Arrange:
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		auto replicatorKey = test::GenerateRandomByteArray<Key>();
		auto fileHash = test::GenerateRandomByteArray<Hash256>();
		state::ReplicatorInfo replicatorInfo{ Height(1), Height(0), { fileHash }, {} };
		entry.replicators().emplace(replicatorKey, replicatorInfo);

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Hash_Redundant,
			entry,
			replicatorKey,
			{ { fileHash }, { fileHash } });
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		auto replicatorKey = test::GenerateRandomByteArray<Key>();
		auto fileHash = test::GenerateRandomByteArray<Hash256>();
		state::ReplicatorInfo replicatorInfo{ Height(1), Height(0), { fileHash }, {} };
		entry.replicators().emplace(replicatorKey, replicatorInfo);

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			entry,
			replicatorKey,
			{ { fileHash } });
	}
}}
