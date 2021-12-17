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
        constexpr auto Current_Height = Height(10);

        auto CreateConfig() {
            test::MutableBlockchainConfiguration config;
            auto pluginConfig = config::StorageConfiguration::Uninitialized();
            pluginConfig.MinDriveSize = utils::FileSize::FromMegabytes(30);
            pluginConfig.MaxDriveSize = utils::FileSize::FromTerabytes(30);
            pluginConfig.MinReplicatorCount = 5;
            config.Network.SetPluginConfiguration(pluginConfig);
            return (config.ToConst());
        }

		auto RandomDriveSize() {
			const auto storageConfig = CreateConfig().Network.template GetPluginConfiguration<config::StorageConfiguration>();
			return test::RandomInRange<uint64_t>(storageConfig.MinDriveSize.megabytes(), storageConfig.MaxDriveSize.megabytes());
		}

        void AssertValidationResult(
                ValidationResult expectedResult,
				const state::BcDriveEntry& driveEntry,
                const state::ReplicatorEntry& replicatorEntry,
                const Key& driveKey,
                const std::shared_ptr<cache::ReplicatorKeyCollector>& replicatorKeyCollector) {
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
            Notification notification(driveEntry.owner(), replicatorEntry.key(), driveEntry.size(), driveEntry.replicatorCount());
            auto pValidator = CreatePrepareDriveValidator(replicatorKeyCollector);
            
            // Act:
            auto result = test::ValidateNotification(*pValidator, notification, cache, CreateConfig(), Current_Height);

			// Assert:
			EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, FailureWhenDriveSizeInsufficient) {
		// Arrange:
        auto driveKey = test::GenerateRandomByteArray<Key>();
        auto Replicator_Key_Collector = std::make_shared<cache::ReplicatorKeyCollector>();
		state::BcDriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
        driveEntry.setSize(0);
        state::ReplicatorEntry replicatorEntry(driveKey);
        Replicator_Key_Collector->addKey(replicatorEntry);
        replicatorEntry.drives().emplace(*Replicator_Key_Collector->keys().begin(), state::DriveInfo());

		// Assert:
		AssertValidationResult(
            Failure_Storage_Drive_Size_Insufficient,
			driveEntry,
            replicatorEntry,
            test::GenerateRandomByteArray<Key>(),
            Replicator_Key_Collector);
	}

	TEST(TEST_CLASS, FailureWhenDriveSizeExcessive) {
    	// Arrange:
    	auto driveKey = test::GenerateRandomByteArray<Key>();
    	auto Replicator_Key_Collector = std::make_shared<cache::ReplicatorKeyCollector>();
    	state::BcDriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
    	driveEntry.setSize(1e9);
    	state::ReplicatorEntry replicatorEntry(driveKey);
    	Replicator_Key_Collector->addKey(replicatorEntry);
    	replicatorEntry.drives().emplace(*Replicator_Key_Collector->keys().begin(), state::DriveInfo());

    	// Assert:
    	AssertValidationResult(
    			Failure_Storage_Drive_Size_Excessive,
    			driveEntry,
    			replicatorEntry,
    			test::GenerateRandomByteArray<Key>(),
    			Replicator_Key_Collector);
    }

    TEST(TEST_CLASS, FailureWhenReplicatorCountInsufficient) {  
        // Arrange:
        auto Replicator_Key_Collector = std::make_shared<cache::ReplicatorKeyCollector>();
        state::BcDriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
        driveEntry.setSize(RandomDriveSize());
        driveEntry.setReplicatorCount(2);
        state::ReplicatorEntry replicatorEntry(test::GenerateRandomByteArray<Key>());
        Replicator_Key_Collector->addKey(replicatorEntry);
        replicatorEntry.drives().emplace(*Replicator_Key_Collector->keys().begin(), state::DriveInfo());

        // Assert:
        AssertValidationResult(
            Failure_Storage_Replicator_Count_Insufficient,
            driveEntry,
            replicatorEntry,
            test::GenerateRandomByteArray<Key>(),
            Replicator_Key_Collector);
    }

    TEST(TEST_CLASS, FailureWhenDriveAlreadyExists) {
        // Arrange:
        auto driveKey = test::GenerateRandomByteArray<Key>();
        auto Replicator_Key_Collector = std::make_shared<cache::ReplicatorKeyCollector>();
        state::BcDriveEntry driveEntry(driveKey);
        driveEntry.setSize(RandomDriveSize());
        driveEntry.setReplicatorCount(Replicator_Count);
        state::ReplicatorEntry replicatorEntry(driveKey);
        Replicator_Key_Collector->addKey(replicatorEntry);
        replicatorEntry.drives().emplace(*Replicator_Key_Collector->keys().begin(), state::DriveInfo());

        // Assert:
        AssertValidationResult(
            Failure_Storage_Drive_Already_Exists,
            driveEntry,
            replicatorEntry,
            driveKey,
            Replicator_Key_Collector);
    }

    TEST(TEST_CLASS, FailureWhenNoReplicator) { 
        // Arrange:
        auto Replicator_Key_Collector = std::make_shared<cache::ReplicatorKeyCollector>();
        state::BcDriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
        driveEntry.setSize(RandomDriveSize());
        driveEntry.setReplicatorCount(Replicator_Count);
        state::ReplicatorEntry replicatorEntry(test::GenerateRandomByteArray<Key>());

        // Assert:
        AssertValidationResult(
            Failure_Storage_No_Replicator,
            driveEntry,
            replicatorEntry,
            test::GenerateRandomByteArray<Key>(),
            Replicator_Key_Collector);
    }

    TEST(TEST_CLASS, FailureWhenReplicatorNotFound) { 
        // Arrange:
        auto Replicator_Key_Collector = std::make_shared<cache::ReplicatorKeyCollector>();
        state::BcDriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
        driveEntry.setSize(RandomDriveSize());
        driveEntry.setReplicatorCount(Replicator_Count);
        state::ReplicatorEntry replicatorEntry(test::GenerateRandomByteArray<Key>());
        Replicator_Key_Collector->addKey(state::ReplicatorEntry(test::GenerateRandomByteArray<Key>()));
        
        // Assert:
        AssertValidationResult(
            Failure_Storage_Replicator_Not_Found,
            driveEntry,
            replicatorEntry,
            test::GenerateRandomByteArray<Key>(),
            Replicator_Key_Collector);
    }

    TEST(TEST_CLASS, Success) {
		// Arrange:
        auto Replicator_Key_Collector = std::make_shared<cache::ReplicatorKeyCollector>();
        auto driveKey = test::GenerateRandomByteArray<Key>();
        state::BcDriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
        driveEntry.setSize(RandomDriveSize());
        driveEntry.setReplicatorCount(Replicator_Count);
        state::ReplicatorEntry replicatorEntry(driveKey);
        Replicator_Key_Collector->addKey(replicatorEntry);
        replicatorEntry.drives().emplace(*Replicator_Key_Collector->keys().begin(), state::DriveInfo());

        // Assert:
		AssertValidationResult(
			ValidationResult::Success,
			driveEntry,
            replicatorEntry,
            test::GenerateRandomByteArray<Key>(),
            Replicator_Key_Collector);
	}
}}