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

#define TEST_CLASS PrepareDriveValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(PrepareDrive, std::make_shared<cache::ReplicatorKeyCollector>())

    namespace {
        using Notification = model::PrepareDriveNotification<1>;

        constexpr auto Replicator_Count = 5;
        const auto Replicator_Key_Collector = std::make_shared<cache::ReplicatorKeyCollector>();
        constexpr auto Current_Height = Height(10);

        void AssertValidationResult(
                ValidationResult expectedResult,
				const state::BcDriveEntry& driveEntry,
                const state::ReplicatorEntry& replicatorEntry) {
            // Arrange:
            auto cache = test::BcDriveCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto& driveCacheDelta = delta.sub<cache::BcDriveCache>();
                driveCacheDelta.insert(driveEntry);
                auto& replicatorCacheDelta = delta.sub<cache::ReplicatorCache>();
                replicatorCacheDelta.insert(replicatorEntry);
                cache.commit(Current_Height);
            }
            Notification notification(driveEntry.owner(), driveEntry.key(), driveEntry.size(), Replicator_Count);
            auto pValidator = CreatePrepareDriveValidator(Replicator_Key_Collector);
            
            // Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, 
                config::BlockchainConfiguration::Uninitialized(), Current_Height);

			// Assert:
			EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, FailureWhenDriveSizeInsufficient) {
		// Arrange:
        auto driveKey = test::GenerateRandomByteArray<Key>();
		state::BcDriveEntry driveEntry(driveKey);
        driveEntry.setSize(0);
        state::ReplicatorEntry replicatorEntry(driveKey);

		// Assert:
		AssertValidationResult(
            Failure_Storage_Drive_Size_Insufficient,
			driveEntry,
            replicatorEntry);
	}

    TEST(TEST_CLASS, FailureWhenReplicatorCountInsufficient) {
        // Arrange:
        auto driveKey = test::GenerateRandomByteArray<Key>();
        state::BcDriveEntry driveEntry(driveKey);
        driveEntry.setReplicatorCount(2);
        state::ReplicatorEntry replicatorEntry(driveKey);

        // Assert:
        AssertValidationResult(
            Failure_Storage_Replicator_Count_Insufficient,
            driveEntry,
            replicatorEntry);
    }

    TEST(TEST_CLASS, FailureWhenDriveAlreadyExists) {
        // Arrange:
        auto driveKey = test::GenerateRandomByteArray<Key>();
        state::BcDriveEntry driveEntry(driveKey);
        state::ReplicatorEntry replicatorEntry(driveKey);

        // Assert:
        AssertValidationResult(
            Failure_Storage_Drive_Already_Exists,
            driveEntry,
            replicatorEntry);
    }

    TEST(TEST_CLASS, FailureWhenNoReplicator) {
        // Arrange:
        auto driveKey = test::GenerateRandomByteArray<Key>();
        state::BcDriveEntry driveEntry(driveKey);
        state::ReplicatorEntry replicatorEntry(driveKey);

        // Assert:
        AssertValidationResult(
            Failure_Storage_No_Replicator,
            driveEntry,
            replicatorEntry);
    }

    TEST(TEST_CLASS, FailureWhenMultipleReplicators) {
        // Arrange:
        auto driveKey = test::GenerateRandomByteArray<Key>();
        state::BcDriveEntry driveEntry(driveKey);
        state::ReplicatorEntry replicatorEntry(driveKey);

        // Assert:
        AssertValidationResult(
            Failure_Storage_Multiple_Replicators,
            driveEntry,
            replicatorEntry);
    }

    TEST(TEST_CLASS, FailureWhenReplicatorNotFound) {
        // Arrange:
        auto driveKey = test::GenerateRandomByteArray<Key>();
        state::BcDriveEntry driveEntry(driveKey);
        state::ReplicatorEntry replicatorEntry(driveKey);

        // Assert:
        AssertValidationResult(
            Failure_Storage_Replicator_Not_Found,
            driveEntry,
            replicatorEntry);
    }

    TEST(TEST_CLASS, Success) {
		// Arrange:
        auto driveKey = test::GenerateRandomByteArray<Key>();
        state::BcDriveEntry driveEntry(driveKey);
        state::ReplicatorEntry replicatorEntry(driveKey);
        replicatorEntry.key();

        // Assert:
		AssertValidationResult(
			ValidationResult::Success,
			driveEntry,
            replicatorEntry);
	}
}}