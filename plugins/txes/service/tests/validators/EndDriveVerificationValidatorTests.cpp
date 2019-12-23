/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "src/cache/DriveCache.h"
#include "src/validators/Validators.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS EndDriveVerificationValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(EndDriveVerification, )

	namespace {
		using Notification = model::EndDriveVerificationNotification<1>;

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
				const state::SecretLockInfo& secretLockInfo,
				const std::vector<model::VerificationFailure>& failures = {}) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::DriveCacheFactory::Create();
			{
				auto delta = cache.createDelta();

				auto& driveCacheDelta = delta.sub<cache::DriveCache>();
				driveCacheDelta.insert(driveEntry);

				auto& secretLockCacheDelta = delta.template sub<cache::SecretLockInfoCache>();
				secretLockCacheDelta.insert(secretLockInfo);

				cache.commit(currentHeight);
			}
			Notification notification(driveEntry.key(), failures.size(), failures.data());
			auto pValidator = CreateEndDriveVerificationValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenVerificationHasNotStarted) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		auto secretLockInfo = CreateLockInfo(test::GenerateRandomByteArray<Key>());

		// Assert:
		AssertValidationResult(
			Failure_Service_Verification_Has_Not_Started,
			driveEntry,
			secretLockInfo);
	}

	TEST(TEST_CLASS, FailureWhenVerificationIsNotActive) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		auto secretLockInfo = CreateLockInfo(driveKey);
		secretLockInfo.Status = state::LockStatus::Used;

		// Assert:
		AssertValidationResult(
			Failure_Service_Verification_Is_Not_Active,
			driveEntry,
			secretLockInfo);
	}

	TEST(TEST_CLASS, FailureWhenReplicatorNotRegistered) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		auto secretLockInfo = CreateLockInfo(driveKey);

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Replicator_Not_Registered,
			driveEntry,
			secretLockInfo,
			{ { test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Hash256>() } });
	}

	TEST(TEST_CLASS, FailureWhenParticipantRedundant) {
		// Arrange:
		auto replicatorKey = test::GenerateRandomByteArray<Key>();
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.replicators().emplace(replicatorKey, state::ReplicatorInfo{ Height(100), Height(1), {}, {} });
		auto secretLockInfo = CreateLockInfo(driveKey);

		// Assert:
		AssertValidationResult(
			Failure_Service_Participant_Redundant,
			driveEntry,
			secretLockInfo,
			{ { replicatorKey, test::GenerateRandomByteArray<Hash256>() },  { replicatorKey, test::GenerateRandomByteArray<Hash256>() } });
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		auto replicatorKey = test::GenerateRandomByteArray<Key>();
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.replicators().emplace(replicatorKey, state::ReplicatorInfo{ Height(100), Height(1), {}, {} });
		auto secretLockInfo = CreateLockInfo(driveKey);

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			driveEntry,
			secretLockInfo,
			{ { replicatorKey, test::GenerateRandomByteArray<Hash256>() } });
	}
}}
