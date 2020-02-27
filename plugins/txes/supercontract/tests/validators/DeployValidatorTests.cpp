/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/SuperContractCache.h"
#include "src/validators/Validators.h"
#include "plugins/txes/service/src/cache/DriveCache.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS DeployValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(Deploy, )

	constexpr auto Current_Height = Height(10);

	namespace {
		using Notification = model::DeployNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const Key& superContract,
				const Key& owner,
				const Key& drive,
				const Hash256& file,
				const state::DriveEntry& driveEntry,
				const state::SuperContractEntry* pSuperContractEntry = nullptr) {
			// Arrange:
			auto cache = test::SuperContractCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& driveCacheDelta = delta.sub<cache::DriveCache>();
				driveCacheDelta.insert(driveEntry);
				if (!!pSuperContractEntry) {
					auto &superContractCacheDelta = delta.sub<cache::SuperContractCache>();
					superContractCacheDelta.insert(*pSuperContractEntry);
				}
				cache.commit(Current_Height);
			}
			Notification notification(superContract,owner, drive, file, test::GenerateRandomValue<VmVersion>());
			auto pValidator = CreateDeployValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
				config::BlockchainConfiguration::Uninitialized(), Current_Height);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenSuperContractExists) {
		// Arrange:
		auto drive = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(drive);
		auto superContract = test::GenerateRandomByteArray<Key>();
		state::SuperContractEntry superContractEntry(superContract);

		// Assert:
		AssertValidationResult(
			Failure_SuperContract_Super_Contract_Already_Exists,
			superContract,
			test::GenerateRandomByteArray<Key>(),
			drive,
			test::GenerateRandomByteArray<Hash256>(),
			driveEntry,
			&superContractEntry);
	}

	TEST(TEST_CLASS, FailureWhenOwnerInvalid) {
		// Arrange:
		auto drive = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(drive);

		// Assert:
		AssertValidationResult(
			Failure_SuperContract_Operation_Is_Not_Permitted,
			test::GenerateRandomByteArray<Key>(),
			test::GenerateRandomByteArray<Key>(),
			drive,
			test::GenerateRandomByteArray<Hash256>(),
			driveEntry);
	}

	TEST(TEST_CLASS, FailureWhenSuperContractFileDoesNotExist) {
		// Arrange:
		auto drive = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(drive);
		auto owner = test::GenerateRandomByteArray<Key>();
		driveEntry.coowners().insert(owner);

		// Assert:
		AssertValidationResult(
			Failure_SuperContract_File_Does_Not_Exist,
			test::GenerateRandomByteArray<Key>(),
			owner,
			drive,
			test::GenerateRandomByteArray<Hash256>(),
			driveEntry);
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		auto drive = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(drive);
		auto owner = test::GenerateRandomByteArray<Key>();
		driveEntry.coowners().insert(owner);
		auto file = test::GenerateRandomByteArray<Hash256>();
		driveEntry.files().emplace(file, state::FileInfo{ 1000 });

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			test::GenerateRandomByteArray<Key>(),
			owner,
			drive,
			file,
			driveEntry);
	}
}}
