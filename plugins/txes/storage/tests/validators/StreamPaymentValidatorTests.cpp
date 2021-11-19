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

#define TEST_CLASS StreamPaymentValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(StreamPayment, )

    namespace {
        using Notification = model::StreamPaymentNotification<1>;

        constexpr auto Current_Height = Height(10);

        auto CreateConfig() {
        	test::MutableBlockchainConfiguration config;
        	auto pluginConfig = config::StorageConfiguration::Uninitialized();
        	pluginConfig.MaxModificationSize = utils::FileSize::FromMegabytes(30);
        	config.Network.SetPluginConfiguration(pluginConfig);
        	return (config.ToConst());
        }

        void AssertValidationResult(
                ValidationResult expectedResult,
                const state::BcDriveEntry& driveEntry,
                const Key& driveKey,
				const Hash256& streamId,
				const uint64_t& additionalSize) {
            // Arrange:
            auto cache = test::BcDriveCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto& driveCacheDelta = delta.sub<cache::BcDriveCache>();
                driveCacheDelta.insert(driveEntry);
                cache.commit(Current_Height);
            }
        	Notification notification(driveKey, streamId, additionalSize);
            auto pValidator = CreateStreamPaymentValidator();

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
            test::GenerateRandomByteArray<Hash256>(),
			test::Random());
    }

    TEST(TEST_CLASS, FailureWhenUploadSizeExcessive) {
    	// Arrange:
    	Key driveKey = test::GenerateRandomByteArray<Key>();
    	Key owner = test::GenerateRandomByteArray<Key>();
    	uint64_t expectedUploadSize = 1e9;
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
    			Failure_Storage_Upload_Size_Excessive,
    			entry,
    			driveKey,
    			streamId,
    			test::Random());
    }

    TEST(TEST_CLASS, FailureWhenInvalidStreamId) {
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
			Failure_Storage_Invalid_Stream_Id,
			entry,
			driveKey,
			test::GenerateRandomByteArray<Hash256>(),
			test::Random());
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
			streamId,
			test::Random());
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
			streamId,
		  	test::Random());
    }
}}