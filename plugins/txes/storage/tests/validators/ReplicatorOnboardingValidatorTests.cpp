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

    DEFINE_COMMON_VALIDATOR_TESTS(ReplicatorOnboarding, )

    namespace {
        using Notification = model::ReplicatorOnboardingNotification<1>;

        constexpr auto Current_Height = Height(10);

        void AssertValidationResult(
                ValidationResult expectedResult,
				const state::ReplicatorEntry& replicatorEntry,
				const state::BlsKeysEntry& blsEntry,
                const Key& driveKey) {
            // Arrange:
            auto cache = test::ReplicatorCacheFactory::Create();
            {
				auto delta = cache.createDelta();
                auto& replicatorCacheDelta = delta.sub<cache::ReplicatorCache>();
                replicatorCacheDelta.insert(replicatorEntry);
				cache.commit(Current_Height);
			}
			{
				auto delta = cache.createDelta();
				auto& blsCacheDela = delta.sub<cache::BlsKeysCache>();
				blsCacheDela.insert(blsEntry);
				cache.commit(Current_Height);
			}

            Notification notification(driveKey, BLSPublicKey(), replicatorEntry.capacity());
            auto pValidator = CreateReplicatorOnboardingValidator();

            // Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
                config::BlockchainConfiguration::Uninitialized(), Current_Height);

			// Assert:
			EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, FailureWhenReplicatorAlreadyRegistered) {
        // Arrange:
        Key driveKey = test::GenerateRandomByteArray<Key>();
        state::ReplicatorEntry replicatorEntry(driveKey);
		state::BlsKeysEntry blsKeysEntry(test::GenerateRandomByteArray<BLSPublicKey>());

        // Assert:
        AssertValidationResult(
            Failure_Storage_Replicator_Already_Registered,
            replicatorEntry,
			blsKeysEntry,
            driveKey);
    }

    TEST(TEST_CLASS, Success) {
		// Arrange:
        state::ReplicatorEntry replicatorEntry(test::GenerateRandomByteArray<Key>());
		state::BlsKeysEntry blsKeysEntry(test::GenerateRandomByteArray<BLSPublicKey>());

        // Assert:
		AssertValidationResult(
			ValidationResult::Success,
            replicatorEntry,
			blsKeysEntry,
            test::GenerateRandomByteArray<Key>());
	}
}}