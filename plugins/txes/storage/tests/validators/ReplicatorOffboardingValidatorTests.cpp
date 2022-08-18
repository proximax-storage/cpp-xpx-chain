/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "catapult/model/StorageNotifications.h"
#include "tests/test/StorageTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS ReplicatorOffboardingValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(ReplicatorOffboarding, )

    namespace {
        using Notification = model::ReplicatorOffboardingNotification<1>;

        constexpr auto Current_Height = Height(10);
		constexpr auto Min_Replicator_Count = 4;

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			auto pluginConfig = config::StorageConfiguration::Uninitialized();
			pluginConfig.MinReplicatorCount = Min_Replicator_Count;
			config.Network.SetPluginConfiguration(pluginConfig);
			return (config.ToConst());
		}

        void AssertValidationResult(
                ValidationResult expectedResult,
				const state::ReplicatorEntry& replicatorEntry,
				const state::BcDriveEntry& driveEntry,
				const Key& replicatorKey,
                const Key& driveKey) {
            // Arrange:
            auto cache = test::ReplicatorCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto& replicatorCacheDelta = delta.sub<cache::ReplicatorCache>();
                replicatorCacheDelta.insert(replicatorEntry);
				auto& driveCacheDelta = delta.sub<cache::BcDriveCache>();
				driveCacheDelta.insert(driveEntry);
                cache.commit(Current_Height);
            }
            Notification notification(replicatorKey, driveKey);
            auto pValidator = CreateReplicatorOffboardingValidator();
            
            // Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					CreateConfig(), Current_Height);

			// Assert:
			EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, Success) {
        // Arrange:
		Key replicatorKey = test::GenerateRandomByteArray<Key>();
		Key driveKey = test::GenerateRandomByteArray<Key>();
		state::ReplicatorEntry replicatorEntry(replicatorKey);
		state::BcDriveEntry driveEntry(driveKey);
		driveEntry.replicators().emplace(replicatorKey);
		for (auto i = 1u; i < Min_Replicator_Count; ++i)
			driveEntry.replicators().emplace(test::GenerateRandomByteArray<Key>());

        // Assert:
        AssertValidationResult(
				ValidationResult::Success,
            	replicatorEntry,
				driveEntry,
				replicatorKey,
				driveKey);
    }

    TEST(TEST_CLASS, FailureWhenReplicatorNotRegistered) {
		// Assert:
		AssertValidationResult(
				Failure_Storage_Replicator_Not_Registered,
				test::CreateReplicatorEntry(),
				test::CreateBcDriveEntry(),
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomByteArray<Key>());
	}

	TEST(TEST_CLASS, FailureWhenDriveNotFound) {
		// Arrange:
		Key replicatorKey = test::GenerateRandomByteArray<Key>();
		state::ReplicatorEntry replicatorEntry(replicatorKey);

		// Assert:
		AssertValidationResult(
				Failure_Storage_Drive_Not_Found,
				replicatorEntry,
				test::CreateBcDriveEntry(),
				replicatorKey,
				test::GenerateRandomByteArray<Key>());
	}

	TEST(TEST_CLASS, FailureWhenDriveNotAssignedToReplicator) {
		// Arrange:
		Key replicatorKey = test::GenerateRandomByteArray<Key>();
		Key driveKey = test::GenerateRandomByteArray<Key>();
		state::ReplicatorEntry replicatorEntry(replicatorKey);
		state::BcDriveEntry driveEntry(driveKey);

		// Assert:
		AssertValidationResult(
				Failure_Storage_Drive_Not_Assigned_To_Replicator,
				replicatorEntry,
				driveEntry,
				replicatorKey,
				driveKey);
	}

	TEST(TEST_CLASS, FailureWhenAlreadyAppliedForOffboarding) {
		// Arrange:
		Key replicatorKey = test::GenerateRandomByteArray<Key>();
		Key driveKey = test::GenerateRandomByteArray<Key>();
		state::ReplicatorEntry replicatorEntry(replicatorKey);
		state::BcDriveEntry driveEntry(driveKey);
		driveEntry.replicators().emplace(replicatorKey);
		driveEntry.offboardingReplicators().emplace_back(replicatorKey);

		// Assert:
		AssertValidationResult(
				Failure_Storage_Already_Applied_For_Offboarding,
				replicatorEntry,
				driveEntry,
				replicatorKey,
				driveKey);
	}

}}