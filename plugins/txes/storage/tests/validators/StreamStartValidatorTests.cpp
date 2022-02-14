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

        auto CreateConfig() {
        	test::MutableBlockchainConfiguration config;
        	auto pluginConfig = config::StorageConfiguration::Uninitialized();
        	pluginConfig.MaxModificationSize = utils::FileSize::FromMegabytes(30);
        	config.Network.SetPluginConfiguration(pluginConfig);
        	return (config.ToConst());
        }

		auto RandomUploadSize() {
			const auto config = CreateConfig();
			const auto maxModificationSize = config.Network.template GetPluginConfiguration<config::StorageConfiguration>().MaxModificationSize;
			return test::RandomInRange<uint64_t>(1, maxModificationSize.megabytes());
		}

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
            Notification notification(modification.Id, driveKey, modification.Owner, modification.ExpectedUploadSizeMegabytes, modification.FolderName);
            auto pValidator = CreateStreamStartValidator();
            
            // Act:
            auto result = test::ValidateNotification(*pValidator, notification, cache, 
                CreateConfig(), Current_Height);

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
					RandomUploadSize(),
					std::string(folderNameBytes.begin(), folderNameBytes.end())
				)
			});
    }

    TEST(TEST_CLASS, FailureWhenUploadSizeExcessive) {
    	// Arrange:
    	Key driveKey = test::GenerateRandomByteArray<Key>();
    	Key owner = test::GenerateRandomByteArray<Key>();
    	uint64_t expectedUploadSize = 1e9;
    	state::BcDriveEntry entry(driveKey);
    	auto folderNameBytes = test::GenerateRandomVector(512);
    	entry.activeDataModifications().emplace_back(state::ActiveDataModification(
    			test::GenerateRandomByteArray<Hash256>(), owner, expectedUploadSize,
    			std::string(folderNameBytes.begin(), folderNameBytes.end())
    			));

    	// Assert:
    	AssertValidationResult(
    			Failure_Storage_Upload_Size_Excessive,
    			entry,
    			driveKey,
    			{
    				{ test::GenerateRandomByteArray<Hash256>(), owner, expectedUploadSize,
					  std::string(folderNameBytes.begin(), folderNameBytes.end())}
    			});
    }

    TEST(TEST_CLASS, FailureWhenStreamAlreadyExists) {
        // Arrange:
        auto driveKey = test::GenerateRandomByteArray<Key>();
		auto owner = test::GenerateRandomByteArray<Key>();
        auto id = test::GenerateRandomByteArray<Hash256>();
        state::BcDriveEntry entry(driveKey);
		entry.setOwner(owner);
        entry.activeDataModifications().emplace_back(state::ActiveDataModification {
            id, owner, test::GenerateRandomByteArray<Hash256>(), test::Random()
        });
		auto folderNameBytes = test::GenerateRandomVector(512);

        // Assert:
        AssertValidationResult(
            Failure_Storage_Stream_Already_Exists,
            entry,
            driveKey,
            {state::ActiveDataModification(
				id,
				owner,
				RandomUploadSize(),
				std::string(folderNameBytes.begin(), folderNameBytes.end())
			)}
		);
    }

    TEST(TEST_CLASS, FailureWhenIsNotOwner) {
    	// Arrange:
    	Key driveKey = test::GenerateRandomByteArray<Key>();
    	Key owner = test::GenerateRandomByteArray<Key>();
    	uint64_t expectedUploadSize = RandomUploadSize();
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
        uint64_t expectedUploadSize = RandomUploadSize();
        state::BcDriveEntry entry(driveKey);
		entry.setOwner(owner);
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