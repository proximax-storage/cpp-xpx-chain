/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "src/validators/Validators.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS StartDriveVerificationValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(StartDriveVerification, )

	namespace {
		using Notification = model::StartDriveVerificationNotification<1>;

		constexpr MosaicId Storage_Mosaic_Id(1234);
		constexpr auto Network_Identifier = model::NetworkIdentifier::Zero;

		auto CreateLockInfo(const Key& key) {
			auto driveAddress = model::PublicKeyToAddress(key, Network_Identifier);
			state::SecretLockInfo secretLockInfo(
				key,
				Storage_Mosaic_Id,
				Amount(10),
				Height(15),
				model::LockHashAlgorithm::Op_Internal,
				Hash256(),
				driveAddress);
			secretLockInfo.CompositeHash = model::CalculateSecretLockInfoHash(Hash256(), driveAddress);

			return secretLockInfo;
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::DriveEntry& driveEntry,
				const Key& initiatorKey,
				const state::SecretLockInfo& secretLockInfo) {
			// Arrange:
			Height currentHeight(10);
			auto cache = test::DriveCacheFactory::Create();
			{
				auto delta = cache.createDelta();

				auto& driveCacheDelta = delta.sub<cache::DriveCache>();
				driveCacheDelta.insert(driveEntry);

				auto& secretLockCacheDelta = delta.template sub<cache::SecretLockInfoCache>();
				secretLockCacheDelta.insert(secretLockInfo);

				cache.commit(currentHeight);
			}
			Notification notification(driveEntry.key(), initiatorKey);
			auto pValidator = CreateStartDriveVerificationValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenDriveIsNotInProgress) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.setOwner(test::GenerateRandomByteArray<Key>());
		auto secretLockInfo = CreateLockInfo(driveKey);

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Is_Not_In_Progress,
			driveEntry,
			driveEntry.owner(),
			secretLockInfo);
	}

	TEST(TEST_CLASS, FailureWhenVerificationAlreadyInProgress) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.setState(state::DriveState::InProgress);
		auto secretLockInfo = CreateLockInfo(driveKey);

		// Assert:
		AssertValidationResult(
			Failure_Service_Verification_Already_In_Progress,
			driveEntry,
			driveEntry.owner(),
			secretLockInfo);
	}

	TEST(TEST_CLASS, FailureWhenVerificationHasNotTimedOut) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.setState(state::DriveState::InProgress);
		auto secretLockInfo = CreateLockInfo(driveKey);
		secretLockInfo.Status = state::LockStatus::Used;

		// Assert:
		AssertValidationResult(
			Failure_Service_Verification_Has_Not_Timed_Out,
			driveEntry,
			driveEntry.owner(),
			secretLockInfo);
	}

	TEST(TEST_CLASS, FailureWhenOperationIsNotPermitted) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.setState(state::DriveState::InProgress);
		auto secretLockInfo = CreateLockInfo(test::GenerateRandomByteArray<Key>());

		// Assert:
		AssertValidationResult(
			Failure_Service_Operation_Is_Not_Permitted,
			driveEntry,
			test::GenerateRandomByteArray<Key>(),
			secretLockInfo);
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.setState(state::DriveState::InProgress);
		auto secretLockInfo = CreateLockInfo(test::GenerateRandomByteArray<Key>());

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			driveEntry,
			driveEntry.owner(),
			secretLockInfo);
	}
}}
