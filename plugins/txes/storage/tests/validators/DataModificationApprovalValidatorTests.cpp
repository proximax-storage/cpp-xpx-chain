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
        const auto File_Structure_Cdi =  test::GenerateRandomByteArray<Hash256>();
        constexpr auto File_Structure_Size = 50;
        constexpr auto Used_Drive_Size = 50;

        void AssertValidationResult(
                ValidationResult expectedResult,
                const state::BcDriveEntry& driveEntry,
                const Key& driveKey,
                const std::vector<state::ActiveDataModification>& activeDataModification) {
            // Arrange:
            auto cache = test::BcDriveCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto& driveCacheDelta = delta.sub<cache::BcDriveCache>();
                driveCacheDelta.insert(driveEntry);
                cache.commit(Current_Height);
            }
            Notification notification(Key(), driveKey, activeDataModification.front().Id, activeDataModification.front().DownloadDataCdi, File_Structure_Size, activeDataModification.front().ActualUploadSize);
            auto pValidator = CreateDataModificationApprovalValidator();
            
            // Act:
            auto result = test::ValidateNotification(*pValidator, notification, cache, 
                config::BlockchainConfiguration::Uninitialized(), Current_Height);

            // Assert:
            EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, FailureWhenDriveNotFound) {
        // Arrange:
        state::BcDriveEntry entry(test::GenerateRandomByteArray<Key>());
        entry.activeDataModifications().emplace_back(state::ActiveDataModification {
            test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Key>(), File_Structure_Cdi, Used_Drive_Size
        });

        // Assert:
        AssertValidationResult(
            Failure_Storage_Drive_Not_Found,
            entry,
            test::GenerateRandomByteArray<Key>(),
            { 
                { test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Key>(), File_Structure_Cdi, Used_Drive_Size } 
            });
    }

    TEST(TEST_CLASS, FailureWhenNoActiveDataModification) {
        // Arrange:
        auto driveKey = test::GenerateRandomByteArray<Key>();
        state::BcDriveEntry entry(driveKey);

        // Assert:
        AssertValidationResult(
            Failure_Storage_No_Active_Data_Modifications,
            entry,
            driveKey,
            { 
                { test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Key>(), File_Structure_Cdi, Used_Drive_Size } 
            });
    }

    TEST(TEST_CLASS, FailureWhenInvalidDataModificationId) {
        // Arrange:
        auto driveKey = test::GenerateRandomByteArray<Key>();
        state::BcDriveEntry entry(driveKey);
        entry.activeDataModifications().emplace_back(state::ActiveDataModification {
            test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Key>(), File_Structure_Cdi, Used_Drive_Size
        });

        // Assert:
        AssertValidationResult(
            Failure_Storage_Invalid_Data_Modification_Id,
            entry,
            driveKey,
            {
                { test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Key>(), File_Structure_Cdi, Used_Drive_Size }
            });
    }

    TEST(TEST_CLASS, Success) {
        // Arrange:
        auto driveKey = test::GenerateRandomByteArray<Key>();
        auto Id = test::GenerateRandomByteArray<Hash256>();
        auto ownerPublicKey = test::GenerateRandomByteArray<Key>();
        state::BcDriveEntry entry(driveKey);
        entry.activeDataModifications().emplace_back(state::ActiveDataModification {
            Id, ownerPublicKey, File_Structure_Cdi, Used_Drive_Size
        });

        // Assert:
        AssertValidationResult(
            ValidationResult::Success,
            entry,
            driveKey,
            {
                { Id, ownerPublicKey, File_Structure_Cdi, Used_Drive_Size }
            });
    }
}}