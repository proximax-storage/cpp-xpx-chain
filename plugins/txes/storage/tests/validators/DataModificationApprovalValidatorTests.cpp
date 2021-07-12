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

#define TEST_CLASS DataModificationApprovalValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(DataModificationApproval, )

    namespace {
        using Notification = model::DataModificationApprovalNotification<1>;

        constexpr auto Current_Height = Height(10);
        constexpr auto File_Structure_Size = 50;
        constexpr auto Used_Drive_Size = 50;

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
            Notification notification(driveEntry.key(), driveEntry.activeDataModifications().begin()->Id, driveEntry.activeDataModifications().begin()->DownloadDataCdi, File_Structure_Size, Used_Drive_Size);
            auto pValidator = CreateDataModificationApprovalValidator();
            
            // Act:
            auto result = test::ValidateNotification(*pValidator, notification, cache, 
                config::BlockchainConfiguration::Uninitialized(), Current_Height);

            // Assert:
            EXPECT_EQ(expectedResult, result);
        }
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

    TEST(TEST_CLASS, FailureWhenDataModificationNotFound) {
        // Arrange:
        state::BcDriveEntry entry(test::GenerateRandomByteArray<Key>());
        entry.activeDataModifications().back().Id = test::GenerateRandomByteArray<Hash256>();

        // Assert:
        AssertValidationResult(
            Failure_Storage_Invalid_Data_Modification_Id,
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