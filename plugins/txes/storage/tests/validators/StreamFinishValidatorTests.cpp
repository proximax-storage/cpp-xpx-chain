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

#define TEST_CLASS StreamFinishValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(StreamFinish,)

    namespace {
        using Notification = model::StreamFinishNotification<1>;

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
            Notification notification(driveKey, modification.Id, modification.Owner, modification.ActualUploadSizeMegabytes, modification.DownloadDataCdi);
            auto pValidator = CreateStreamFinishValidator();
            
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

    TEST(TEST_CLASS, FailureWhenNoDataModifications) {
    	// Arrange:
    	Key driveKey = test::GenerateRandomByteArray<Key>();
    	Key owner = test::GenerateRandomByteArray<Key>();
    	uint64_t expectedUploadSize = test::Random();
    	state::BcDriveEntry entry(driveKey);
    	entry.setOwner(owner);
    	auto streamId = test::GenerateRandomByteArray<Hash256>();
    	auto folderNameBytes = test::GenerateRandomVector(512);

    	// Assert:
    	AssertValidationResult(
    			Failure_Storage_No_Active_Data_Modifications,
    			entry,
    			driveKey,
    			{
    				{ streamId, owner, expectedUploadSize,
					  std::string(folderNameBytes.begin(), folderNameBytes.end())}
    			});
    }

    TEST(TEST_CLASS, FailureWhenStreamAlreadyFinished) {
    	// Arrange:
    	Key driveKey = test::GenerateRandomByteArray<Key>();
    	Key owner = test::GenerateRandomByteArray<Key>();
    	uint64_t expectedUploadSize = test::Random();
    	state::BcDriveEntry entry(driveKey);
    	entry.setOwner(owner);
    	auto streamId = test::GenerateRandomByteArray<Hash256>();
    	auto folderNameBytes = test::GenerateRandomVector(512);
    	entry.activeDataModifications().emplace_back(state::ActiveDataModification(
    			streamId, owner, expectedUploadSize,
    			std::string(folderNameBytes.begin(), folderNameBytes.end())
    			));
		entry.activeDataModifications().begin()->ReadyForApproval = true;

    	// Assert:
    	AssertValidationResult(
    			Failure_Storage_Stream_Already_Finished,
    			entry,
    			driveKey,
    			{
    				{ streamId, owner, expectedUploadSize,
					  std::string(folderNameBytes.begin(), folderNameBytes.end())}
    			});
    }

    TEST(TEST_CLASS, FailureWhenIsNotDriveOwner) {
    	// Arrange:
    	Key driveKey = test::GenerateRandomByteArray<Key>();
    	Key owner = test::GenerateRandomByteArray<Key>();
    	uint64_t expectedUploadSize = test::Random();
    	state::BcDriveEntry entry(driveKey);
    	entry.setOwner(owner);
    	auto streamId = test::GenerateRandomByteArray<Hash256>();
    	auto folderNameBytes = test::GenerateRandomVector(512);
    	entry.activeDataModifications().emplace_back(state::ActiveDataModification(
    			streamId, owner, expectedUploadSize,
    			std::string(folderNameBytes.begin(), folderNameBytes.end())
    			));

    	// Assert:
    	AssertValidationResult(
    			Failure_Storage_Is_Not_Owner,
    			entry,
    			driveKey,
    			{
    				state::ActiveDataModification(
    						streamId, test::GenerateRandomByteArray<Key>(), expectedUploadSize,
    						std::string(folderNameBytes.begin(), folderNameBytes.end()))
    			});
    }

    TEST(TEST_CLASS, FailureWhenExceedsExpectedSize) {
    	// Arrange:
    	Key driveKey = test::GenerateRandomByteArray<Key>();
    	Key owner = test::GenerateRandomByteArray<Key>();
    	uint64_t expectedUploadSize = test::Random();
    	state::BcDriveEntry entry(driveKey);
    	entry.setOwner(owner);
    	auto streamId = test::GenerateRandomByteArray<Hash256>();
    	auto folderNameBytes = test::GenerateRandomVector(512);
    	entry.activeDataModifications().emplace_back(state::ActiveDataModification(
    			streamId, owner, expectedUploadSize,
    			std::string(folderNameBytes.begin(), folderNameBytes.end())
    			));

    	// Assert:
    	AssertValidationResult(
    			Failure_Storage_Expected_Upload_Size_Exceeded,
    			entry,
    			driveKey,
    			{
    				state::ActiveDataModification(
    						streamId, owner, test::GenerateRandomByteArray<Hash256>(),
							expectedUploadSize, expectedUploadSize + 1,
							std::string(folderNameBytes.begin(), folderNameBytes.end()), false, true)
    			});
    }

    TEST(TEST_CLASS, Success) {
        // Arrange:
        Key driveKey = test::GenerateRandomByteArray<Key>();
        Key owner = test::GenerateRandomByteArray<Key>();
        uint64_t expectedUploadSize = test::Random();
        state::BcDriveEntry entry(driveKey);
		entry.setOwner(owner);
		auto streamId = test::GenerateRandomByteArray<Hash256>();
		auto folderNameBytes = test::GenerateRandomVector(512);
        entry.activeDataModifications().emplace_back(state::ActiveDataModification(
            streamId, owner, expectedUploadSize,
			std::string(folderNameBytes.begin(), folderNameBytes.end())
		));

        // Assert:
        AssertValidationResult(
            ValidationResult::Success,
            entry,
            driveKey,
            {
            state::ActiveDataModification(
				streamId, owner, expectedUploadSize,
				std::string(folderNameBytes.begin(), folderNameBytes.end()))
            });
    }
}}