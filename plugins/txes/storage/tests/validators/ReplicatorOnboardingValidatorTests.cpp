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
				const std::vector<state::ReplicatorEntry>& replicatorEntry,
                const std::vector<state::BlsKeysEntry>& blsKeysEntry,
                const std::vector<Key> driveKey,
                const std::vector<BLSPublicKey> blsKey) {
            // Arrange:
            auto cache = test::ReplicatorCacheFactory::Create();
            {
                auto entry = replicatorEntry.begin();
                auto blsEntry = blsKeysEntry.begin();
                auto delta = cache.createDelta();
                auto& replicatorCacheDelta = delta.sub<cache::ReplicatorCache>();
                auto& blsKeysCacheDelta = delta.sub<cache::BlsKeysCache>();
                while (entry != replicatorEntry.end() && blsEntry != blsKeysEntry.end()) {
                    replicatorCacheDelta.insert(*entry++);
                    blsKeysCacheDelta.insert(*blsEntry++);
                }
                cache.commit(Current_Height);
            }

            auto entry = replicatorEntry.begin();
            auto blsEntry = blsKeysEntry.begin();
            auto key = driveKey.begin();
            auto bls = blsKey.begin();
            while (entry != replicatorEntry.end() && blsEntry != blsKeysEntry.end() && key != driveKey.end() && bls != blsKey.end()) {
                Notification notification(*key++, *bls++, entry->capacity());
                auto pValidator = CreateReplicatorOnboardingValidator();
                
                // Act:
                auto result = test::ValidateNotification(*pValidator, notification, cache, config::BlockchainConfiguration::Uninitialized(), Current_Height);

                // Assert:
                EXPECT_EQ(expectedResult, result);

                *entry++;
                *blsEntry++;
            }
        }
    }

    TEST(TEST_CLASS, FailureWhenReplicatorAlreadyRegistered) {
        // Arrange:
        Key driveKey = test::GenerateRandomByteArray<Key>();
        BLSPublicKey blsKey = test::GenerateRandomByteArray<BLSPublicKey>();

        state::ReplicatorEntry replicatorEntry(driveKey);
        replicatorEntry.setBlsKey(blsKey);
        
        state::BlsKeysEntry blsKeysEntry(blsKey);
        
        // Assert:
        AssertValidationResult(
            Failure_Storage_Replicator_Already_Registered,
            { replicatorEntry },
            { blsKeysEntry },
            { driveKey },
            { test::GenerateRandomByteArray<BLSPublicKey>() });
    }

    TEST(TEST_CLASS, FailureWhenBLSKeyAlreadyRegistered) {
        // Arrange:
        BLSPublicKey blsKey = test::GenerateRandomByteArray<BLSPublicKey>();
        
        state::ReplicatorEntry replicatorEntry(test::GenerateRandomByteArray<Key>());
        replicatorEntry.setBlsKey(blsKey);

        state::BlsKeysEntry blsKeysEntry(blsKey);

        // Assert:
		AssertValidationResult(
			Failure_Storage_BLS_Key_Already_Registered,
            { replicatorEntry },
            { blsKeysEntry },
            { test::GenerateRandomByteArray<Key>() },
            { blsKey });
    }

    TEST(TEST_CLASS, SuccessWhenSingleReplicator) {
		// Arrange:
        BLSPublicKey blsKey = test::GenerateRandomByteArray<BLSPublicKey>();
        
        state::ReplicatorEntry replicatorEntry(test::GenerateRandomByteArray<Key>());
        replicatorEntry.setBlsKey(blsKey);

        state::BlsKeysEntry blsKeysEntry(blsKey);

        // Assert:
		AssertValidationResult(
			ValidationResult::Success,
            { replicatorEntry },
            { blsKeysEntry },
            { test::GenerateRandomByteArray<Key>() },
            { test::GenerateRandomByteArray<BLSPublicKey>() });
	}

    TEST(TEST_CLASS, SuccessWhenMultipleReplicators) {
		// Arrange:
        BLSPublicKey blsKey1 = test::GenerateRandomByteArray<BLSPublicKey>();;
        BLSPublicKey blsKey2 = test::GenerateRandomByteArray<BLSPublicKey>();

        state::ReplicatorEntry replicatorEntry1(test::GenerateRandomByteArray<Key>());
        replicatorEntry1.setBlsKey(blsKey1);

        state::ReplicatorEntry replicatorEntry2(test::GenerateRandomByteArray<Key>());
        replicatorEntry2.setBlsKey(blsKey2);

        state::BlsKeysEntry blsKeysEntry1(blsKey1);
        state::BlsKeysEntry blsKeysEntry2(blsKey2);

        // Assert:
		AssertValidationResult(
			ValidationResult::Success,
            { replicatorEntry1, replicatorEntry2 },
            { blsKeysEntry1, blsKeysEntry2 },
            { test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>() },
            { test::GenerateRandomByteArray<BLSPublicKey>(), test::GenerateRandomByteArray<BLSPublicKey>() });
	}
}}