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

#define TEST_CLASS DataModificationValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(DataModification, )

    namespace {
        using Notification = model::DataModificationNotification<1>;

		constexpr auto Current_Height = Height(10);
		constexpr auto Drive_Size = 100;
		constexpr auto Drive_Used_Size = 80;
		constexpr auto Drive_Free_Size = Drive_Size - Drive_Used_Size;
		constexpr auto Max_Modification_Size = 30;	// Must be greater than Drive_Free_Size

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			auto pluginConfig = config::StorageConfiguration::Uninitialized();
			pluginConfig.MaxModificationSize = utils::FileSize::FromMegabytes(Max_Modification_Size);
			config.Network.SetPluginConfiguration(pluginConfig);
			return (config.ToConst());
		}

		state::BcDriveEntry CreateDriveEntry(
				const Key& driveKey = test::GenerateRandomByteArray<Key>(),
				const Key& owner = test::GenerateRandomByteArray<Key>()) {
			auto entry = test::CreateBcDriveEntry(driveKey, owner);
			entry.setSize(Drive_Size);
			entry.setUsedSizeBytes(Drive_Used_Size);
			return entry;
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
            Notification notification(activeDataModification.front().Id, driveKey, activeDataModification.front().Owner, activeDataModification.front().DownloadDataCdi, activeDataModification.front().ActualUploadSizeMegabytes);
            auto pValidator = CreateDataModificationValidator();
            
            // Act:
            auto result = test::ValidateNotification(*pValidator, notification, cache, 
                CreateConfig(), Current_Height);

            // Assert:
            EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, FailureWhenUploadSizeExcessive) {
    	// Arrange:
    	const auto entry = CreateDriveEntry();

    	// Assert:
    	AssertValidationResult(
    			Failure_Storage_Upload_Size_Excessive,
    			entry,
    			test::GenerateRandomByteArray<Key>(),
    			{
    				state::ActiveDataModification(
    						test::GenerateRandomByteArray<Hash256>(),
    						test::GenerateRandomByteArray<Key>(),
    						test::GenerateRandomByteArray<Hash256>(),
    						1e9)
    			});
    }

    TEST(TEST_CLASS, FailureWhenDriveNotFound) {
        // Arrange:
		const auto entry = CreateDriveEntry();

        // Assert:
        AssertValidationResult(
            Failure_Storage_Drive_Not_Found,
            entry,
            test::GenerateRandomByteArray<Key>(),
            {
                state::ActiveDataModification(
					test::GenerateRandomByteArray<Hash256>(),
					test::GenerateRandomByteArray<Key>(),
					test::GenerateRandomByteArray<Hash256>(),
					test::RandomInRange<uint64_t>(1, Drive_Free_Size))
            });
    }

//	TEST(TEST_CLASS, FailureWhenDriveSizeInsufficient) {
//		// Arrange:
//		Key driveKey = test::GenerateRandomByteArray<Key>();
//		Key owner = test::GenerateRandomByteArray<Key>();
//		Hash256 id = test::GenerateRandomByteArray<Hash256>();
//		uint64_t uploadSize = test::RandomInRange<uint64_t>(Drive_Free_Size + 1, Max_Modification_Size);
//
//		auto entry = CreateDriveEntry(driveKey, owner);
//		entry.activeDataModifications().emplace_back(state::ActiveDataModification(
//				id, owner, test::GenerateRandomByteArray<Hash256>(), uploadSize
//		));
//
//		// Assert:
//		AssertValidationResult(
//				Failure_Storage_Drive_Size_Insufficient,
//				entry,
//				driveKey,
//				{ state::ActiveDataModification(
//						id, owner, test::GenerateRandomByteArray<Hash256>(), uploadSize)
//				});
//	}

    TEST(TEST_CLASS, FailureWhenDataModificationAlreadyExists) {
        // Arrange:
		Key driveKey = test::GenerateRandomByteArray<Key>();
		Key owner = test::GenerateRandomByteArray<Key>();
		Hash256 id = test::GenerateRandomByteArray<Hash256>();
		uint64_t uploadSize = test::RandomInRange<uint64_t>(1, Drive_Free_Size);

		auto entry = CreateDriveEntry(driveKey, owner);
        entry.activeDataModifications().emplace_back(state::ActiveDataModification(
            id, owner, test::GenerateRandomByteArray<Hash256>(), uploadSize
		));

        // Assert:
        AssertValidationResult(
            Failure_Storage_Data_Modification_Already_Exists,
            entry,
            driveKey,
            { state::ActiveDataModification(
				id, owner, test::GenerateRandomByteArray<Hash256>(), uploadSize)
            });
    }

    TEST(TEST_CLASS, FailureWhenIsNotOwner) {
    	// Arrange:
    	Key driveKey = test::GenerateRandomByteArray<Key>();
    	Key owner = test::GenerateRandomByteArray<Key>();
    	Hash256 downloadDataCdi = test::GenerateRandomByteArray<Hash256>();
    	uint64_t uploadSize = test::RandomInRange<uint64_t>(1, Drive_Free_Size);

		auto entry = CreateDriveEntry(driveKey, owner);
    	entry.activeDataModifications().emplace_back(state::ActiveDataModification (
    			test::GenerateRandomByteArray<Hash256>(), owner, downloadDataCdi, uploadSize
    			));

    	// Assert:
    	AssertValidationResult(
    			Failure_Storage_Is_Not_Owner,
    			entry,
    			driveKey,
    			{ state::ActiveDataModification(
    					test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Key>(), downloadDataCdi, uploadSize)
    			});
    }

    TEST(TEST_CLASS, Success) {
        // Arrange:
        Key driveKey = test::GenerateRandomByteArray<Key>();
        Key owner = test::GenerateRandomByteArray<Key>();
        Hash256 downloadDataCdi = test::GenerateRandomByteArray<Hash256>();
        uint64_t uploadSize = test::RandomInRange<uint64_t>(1, Drive_Free_Size);

		auto entry = CreateDriveEntry(driveKey, owner);
        entry.activeDataModifications().emplace_back(state::ActiveDataModification (
            test::GenerateRandomByteArray<Hash256>(), owner, downloadDataCdi, uploadSize
		));

        // Assert:
        AssertValidationResult(
            ValidationResult::Success,
            entry,
            driveKey,
            { state::ActiveDataModification(
                test::GenerateRandomByteArray<Hash256>(), owner, downloadDataCdi, uploadSize)
            });
    }
}}