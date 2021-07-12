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

#define TEST_CLASS DataModificationCancelValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(DataModificationCancel, )

    namespace {
        using Notification = model::DataModificationCancelNotification<1>;

        constexpr auto Current_Height = Height(10);

        void AssertValidationResult(
                ValidationResult expectedResult,
                const state::BcDriveEntry& driveEntry) {
            // Arrange:
            auto cache = test::BcDriveCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto& driveCacheDelta = delta.sub<cache::BcDriveCache>();
                driveCacheDelta.insert(driveEntry);
                cache.commit(Current_Height);
            }
            Notification notification(driveEntry.key(), driveEntry.owner(), driveEntry.activeDataModifications().begin()->Id);
            auto pValidator = CreateDataModificationCancelValidator();
            
            // Act:
            auto result = test::ValidateNotification(*pValidator, notification, cache, 
                config::BlockchainConfiguration::Uninitialized(), Current_Height);

            // Assert:
            EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, FailureWhenIsNotOwner) {
        // Arrange:
        state::BcDriveEntry entry(test::GenerateRandomByteArray<Key>());

        // Assert:
        AssertValidationResult(
            Failure_Storage_Is_Not_Owner,
            entry);
    }

    TEST(TEST_CLASS, FailureWhenNoActiveDataModification) {
        // Arrange:
        state::BcDriveEntry entry(test::GenerateRandomByteArray<Key>());
        entry.activeDataModifications().empty();

        // Assert:
        AssertValidationResult(
            Failure_Storage_No_Active_Data_Modifications,
            entry);
    }

    TEST(TEST_CLASS, FailureWhenDataModificationIsActive) {
        // Arrange:
        state::BcDriveEntry entry(test::GenerateRandomByteArray<Key>());
        entry.activeDataModifications().front().Id = test::GenerateRandomByteArray<Hash256>();

        // Assert:
        AssertValidationResult(
            Failure_Storage_Data_Modification_Is_Active,
            entry);
    }

    TEST(TEST_CLASS, FailureWhenDataModificationNotFound) {
        // Arrange:
        state::BcDriveEntry entry(test::GenerateRandomByteArray<Key>());
        entry.activeDataModifications().front().Id = test::GenerateRandomByteArray<Hash256>();

        // Assert:
        AssertValidationResult(
            Failure_Storage_Data_Modification_Not_Found,
            entry);
    }

    TEST(TEST_CLASS, Success) {
        // Arrange:
        state::BcDriveEntry entry(test::GenerateRandomByteArray<Key>());
        entry.setOwner(test::GenerateRandomByteArray<Key>());
        entry.activeDataModifications().front().Id = test::GenerateRandomByteArray<Hash256>();

        // Assert:
        AssertValidationResult(
            ValidationResult::Success,
            entry);
    }
}}