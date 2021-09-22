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
                const state::BlsKeysEntry& blsKeysEntry,
                const Key& publicKey,
                const BLSPublicKey& blsKey) {
            // Arrange:
            auto cache = test::StorageCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto& replicatorCacheDelta = delta.sub<cache::ReplicatorCache>();
                replicatorCacheDelta.insert(replicatorEntry);
                auto& blsKeysCacheDelta = delta.sub<cache::BlsKeysCache>();
                blsKeysCacheDelta.insert(blsKeysEntry);
                cache.commit(Current_Height);
            }
            Notification notification(publicKey, blsKey, replicatorEntry.capacity());
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
        Key publicKey = test::GenerateRandomByteArray<Key>();
        BLSPublicKey blsKey = test::GenerateRandomByteArray<BLSPublicKey>();

        state::ReplicatorEntry replicatorEntry(publicKey);
        replicatorEntry.setBlsKey(blsKey);
        state::BlsKeysEntry blsEntry(blsKey);
        blsEntry.setKey(publicKey);

        // Assert:
        AssertValidationResult(
            Failure_Storage_Replicator_Already_Registered,
            replicatorEntry,
            blsEntry,
            publicKey,
            test::GenerateRandomByteArray<BLSPublicKey>());
    }

    TEST(TEST_CLASS, FailureWhenBlsAlreadyRegistered) {
        // Arrange:
        BLSPublicKey blsKey = test::GenerateRandomByteArray<BLSPublicKey>();

        state::ReplicatorEntry replicatorEntry(test::GenerateRandomByteArray<Key>());
        replicatorEntry.setBlsKey(blsKey);
        state::BlsKeysEntry blsEntry(blsKey);
        blsEntry.setKey(test::GenerateRandomByteArray<Key>());

        // Assert:
        AssertValidationResult(
            Failure_Storage_BLS_Key_Already_Registered,
            replicatorEntry,
            blsEntry,
            test::GenerateRandomByteArray<Key>(),
            blsKey);
    }

    TEST(TEST_CLASS, Success) {
		// Arrange:
        Key publicKey = test::GenerateRandomByteArray<Key>();
        BLSPublicKey blsKey = test::GenerateRandomByteArray<BLSPublicKey>();
        
        state::ReplicatorEntry replicatorEntry(publicKey);
        replicatorEntry.setBlsKey(blsKey);
        state::BlsKeysEntry blsEntry(blsKey);
        blsEntry.setKey(publicKey);

        // Assert:
		AssertValidationResult(
			ValidationResult::Success,
            replicatorEntry,
            blsEntry,
            test::GenerateRandomByteArray<Key>(),
            test::GenerateRandomByteArray<BLSPublicKey>());
	}
}}