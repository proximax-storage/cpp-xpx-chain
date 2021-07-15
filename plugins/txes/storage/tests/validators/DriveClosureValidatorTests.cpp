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

#define TEST_CLASS DriveClosureValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(DriveClosure, )

    namespace {
        using Notification = model::DriveClosureNotification<1>;

        constexpr auto Current_Height = Height(10);

        void AssertValidationResult(
                ValidationResult expectedResult,
                const state::BcDriveEntry& driveEntry,
                const Key& driveKey) {
            // Arrange:
            auto cache = test::BcDriveCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto& driveCacheDelta = delta.sub<cache::BcDriveCache>();
                driveCacheDelta.insert(driveEntry);
                cache.commit(Current_Height);
            }
            Notification notification(driveKey);
            auto pValidator = CreateDriveClosureValidator();
            
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

        // Assert:
        AssertValidationResult(
            Failure_Storage_Drive_Not_Found,
            entry,
            test::GenerateRandomByteArray<Key>());
    }

    TEST(TEST_CLASS, Success) {
        // Arrange:
        Key driveKey = test::GenerateRandomByteArray<Key>();
        state::BcDriveEntry entry(driveKey);
        
        // Assert:
        AssertValidationResult(
            ValidationResult::Success,
            entry,
            driveKey);
    }
}}