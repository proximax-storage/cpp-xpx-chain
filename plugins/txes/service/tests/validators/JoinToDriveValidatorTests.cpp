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

#define TEST_CLASS JoinToDriveValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(JoinToDrive, )

	namespace {
		using Notification = model::JoinToDriveNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::DriveEntry& entry,
				const Key& replicatorKey) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::DriveCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& driveCacheDelta = delta.sub<cache::DriveCache>();
				driveCacheDelta.insert(entry);
				cache.commit(currentHeight);
			}
			Notification notification(entry.key(), replicatorKey);
			auto pValidator = CreateJoinToDriveValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenReplicatorRegistered) {
		// Arrange:
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setReplicas(1);
		auto replicatorKey = test::GenerateRandomByteArray<Key>();
		state::ReplicatorInfo replicatorInfo{ Height(1), Height(0), {}, {} };
		entry.replicators().emplace(replicatorKey, replicatorInfo);

		// Assert:
		AssertValidationResult(
			Failure_Service_Replicator_Already_Connected_To_Drive,
			entry,
			replicatorKey);
	}

	TEST(TEST_CLASS, FailureWhenMaxReplicatorsReached) {
		// Arrange:
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setReplicas(1);
		auto replicatorKey = test::GenerateRandomByteArray<Key>();
		state::ReplicatorInfo replicatorInfo{ Height(1), Height(0), {}, {} };
		entry.replicators().emplace(replicatorKey, replicatorInfo);

		// Assert:
		AssertValidationResult(
			Failure_Service_Max_Replicators_Reached,
			entry,
			test::GenerateRandomByteArray<Key>());
	}

	TEST(TEST_CLASS, Success) {
		// Assert:
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setReplicas(1);
		AssertValidationResult(
			ValidationResult::Success,
			entry,
			test::GenerateRandomByteArray<Key>());
	}
}}
