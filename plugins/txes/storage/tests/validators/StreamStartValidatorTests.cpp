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

#define TEST_CLASS StreamStartValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(StreamStart, )

    namespace {
        using Notification = model::StreamStartNotification<1>;

        constexpr auto Current_Height = Height(10);

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
			const auto& modification = activeDataModification.front();
            Notification notification(modification.Id, driveKey, modification.Owner, modification.ExpectedUploadSize, modification.FolderName);
            auto pValidator = CreateStreamStartValidator();
            
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
		auto folderNameBytes = test::GenerateRandomVector(512);

        // Assert:
        AssertValidationResult(
            Failure_Storage_Drive_Not_Found,
            entry,
            test::GenerateRandomByteArray<Key>(),
            { 
                state::ActiveDataModification(
					test::GenerateRandomByteArray<Hash256>(),
					test::GenerateRandomByteArray<Key>(),
					test::Random(),
					std::string(folderNameBytes.begin(), folderNameBytes.end())
				)
			});
    }

    TEST(TEST_CLASS, FailureWhenStreamAlreadyExists) {
        // Arrange:
        auto driveKey = test::GenerateRandomByteArray<Key>();
        auto id = test::GenerateRandomByteArray<Hash256>();
        state::BcDriveEntry entry(driveKey);
        entry.activeDataModifications().emplace_back(state::ActiveDataModification {
            id, test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Hash256>(), test::Random()
        });
		auto folderNameBytes = test::GenerateRandomVector(512);

        // Assert:
        AssertValidationResult(
            Failure_Storage_Stream_Already_Exists,
            entry,
            driveKey,
            {state::ActiveDataModification(
				id,
				test::GenerateRandomByteArray<Key>(),
				test::Random(),
				std::string(folderNameBytes.begin(), folderNameBytes.end())
			)}
		);
    }

    TEST(TEST_CLASS, FailureWhenIsNotOwner) {
    	// Arrange:
    	Key driveKey = test::GenerateRandomByteArray<Key>();
    	Key owner = test::GenerateRandomByteArray<Key>();
    	uint64_t expectedUploadSize = test::Random();
    	state::BcDriveEntry entry(driveKey);
    	auto folderNameBytes = test::GenerateRandomVector(512);
    	entry.activeDataModifications().emplace_back(state::ActiveDataModification(
    			test::GenerateRandomByteArray<Hash256>(), owner, expectedUploadSize,
    			std::string(folderNameBytes.begin(), folderNameBytes.end())
    			));

    	// Assert:
    	AssertValidationResult(
    			Failure_Storage_Is_Not_Owner,
    			entry,
    			driveKey,
    			{
    				{ test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Key>(), expectedUploadSize,
					  std::string(folderNameBytes.begin(), folderNameBytes.end())}
    			});
    }

    TEST(TEST_CLASS, Success) {
        // Arrange:
        Key driveKey = test::GenerateRandomByteArray<Key>();
        Key owner = test::GenerateRandomByteArray<Key>();
        uint64_t expectedUploadSize = test::Random();
        state::BcDriveEntry entry(driveKey);
		auto folderNameBytes = test::GenerateRandomVector(512);
        entry.activeDataModifications().emplace_back(state::ActiveDataModification(
            test::GenerateRandomByteArray<Hash256>(), owner, expectedUploadSize,
			std::string(folderNameBytes.begin(), folderNameBytes.end())
		));

        // Assert:
        AssertValidationResult(
            ValidationResult::Success,
            entry,
            driveKey,
            { 
                { test::GenerateRandomByteArray<Hash256>(), owner, expectedUploadSize,
						  std::string(folderNameBytes.begin(), folderNameBytes.end())}
            });
    }
}}