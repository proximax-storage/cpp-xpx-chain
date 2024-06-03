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

#define TEST_CLASS ReplicatorOnboardingValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(ReplicatorOnboardingV1, )
    DEFINE_COMMON_VALIDATOR_TESTS(ReplicatorOnboardingV2, )

    namespace {
        constexpr auto Current_Height = Height(10);
		const Hash256 Hash_Seed = test::GenerateRandomByteArray<Hash256>();

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			auto pluginConfig = config::StorageConfiguration::Uninitialized();
			pluginConfig.MinCapacity = utils::FileSize::FromMegabytes(1);
			config.Network.SetPluginConfiguration(pluginConfig);
			return (config.ToConst());
		}

		template<VersionType version>
        void AssertValidationResult(
				stateful::NotificationValidatorPointerT<model::ReplicatorOnboardingNotification<version>> pValidator,
                ValidationResult expectedResult,
				const state::ReplicatorEntry& replicatorEntry,
                const Key& publicKey,
				const Amount& capacity) {
            // Arrange:
            auto cache = test::StorageCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto& replicatorCacheDelta = delta.sub<cache::ReplicatorCache>();
                replicatorCacheDelta.insert(replicatorEntry);
                cache.commit(Current_Height);
            }
			model::ReplicatorOnboardingNotification<version> notification(publicKey, capacity, Hash_Seed);
            
            // Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, CreateConfig(), Current_Height);

			// Assert:
			EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, FailureWhenReplicatorAlreadyRegisteredV1) {
        // Arrange:
        Key publicKey = test::GenerateRandomByteArray<Key>();
        state::ReplicatorEntry replicatorEntry(publicKey);

        // Assert:
        AssertValidationResult(
			CreateReplicatorOnboardingV1Validator(),
            Failure_Storage_Replicator_Already_Registered,
            replicatorEntry,
            publicKey,
			test::GenerateRandomValue<Amount>());
    }

    TEST(TEST_CLASS, FailureWhenReplicatorAlreadyRegisteredV2) {
        // Arrange:
        Key publicKey = test::GenerateRandomByteArray<Key>();
        state::ReplicatorEntry replicatorEntry(publicKey);

        // Assert:
        AssertValidationResult(
			CreateReplicatorOnboardingV2Validator(),
            Failure_Storage_Replicator_Already_Registered,
            replicatorEntry,
            publicKey,
			test::GenerateRandomValue<Amount>());
    }

	TEST(TEST_CLASS, FailureWhenReplicatorCapacityInsufficientV1) {
		// Arrange:
		Key publicKey = test::GenerateRandomByteArray<Key>();
		state::ReplicatorEntry replicatorEntry(publicKey);

		// Assert:
		AssertValidationResult(
			CreateReplicatorOnboardingV1Validator(),
			Failure_Storage_Replicator_Capacity_Insufficient,
			replicatorEntry,
			test::GenerateRandomByteArray<Key>(),
			Amount(0u));
	}

	TEST(TEST_CLASS, FailureWhenReplicatorCapacityInsufficientV2) {
		// Arrange:
		Key publicKey = test::GenerateRandomByteArray<Key>();
		state::ReplicatorEntry replicatorEntry(publicKey);

		// Assert:
		AssertValidationResult(
			CreateReplicatorOnboardingV2Validator(),
			Failure_Storage_Replicator_Capacity_Insufficient,
			replicatorEntry,
			test::GenerateRandomByteArray<Key>(),
			Amount(0u));
	}

    TEST(TEST_CLASS, SuccessV1) {
		// Arrange:
        Key publicKey = test::GenerateRandomByteArray<Key>();
        state::ReplicatorEntry replicatorEntry(publicKey);

        // Assert:
		AssertValidationResult(
			CreateReplicatorOnboardingV1Validator(),
			ValidationResult::Success,
            replicatorEntry,
            test::GenerateRandomByteArray<Key>(),
			test::GenerateRandomValue<Amount>());
	}

    TEST(TEST_CLASS, SuccessV2) {
		// Arrange:
        Key publicKey = test::GenerateRandomByteArray<Key>();
        state::ReplicatorEntry replicatorEntry(publicKey);

        // Assert:
		AssertValidationResult(
			CreateReplicatorOnboardingV2Validator(),
			ValidationResult::Success,
            replicatorEntry,
            test::GenerateRandomByteArray<Key>(),
			test::GenerateRandomValue<Amount>());
	}
}}