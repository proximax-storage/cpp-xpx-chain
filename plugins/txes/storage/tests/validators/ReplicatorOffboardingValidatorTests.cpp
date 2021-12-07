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

#define TEST_CLASS ReplicatorOffboardingValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(ReplicatorOffboarding, )

    namespace {
        using Notification = model::ReplicatorOffboardingNotification<1>;

        constexpr auto Current_Height = Height(10);

        void AssertValidationResult(
                ValidationResult expectedResult,
				const state::ReplicatorEntry& replicatorEntry,
                const Key& driveKey) {
            // Arrange:
            auto cache = test::ReplicatorCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto& replicatorCacheDelta = delta.sub<cache::ReplicatorCache>();
                replicatorCacheDelta.insert(replicatorEntry);
                cache.commit(Current_Height);
            }
            Notification notification(driveKey);
            auto pValidator = CreateReplicatorOffboardingValidator();
            
            // Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, 
                config::BlockchainConfiguration::Uninitialized(), Current_Height);

			// Assert:
			EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, Success) {
        // Arrange:
		Key replicatorKey = test::GenerateRandomByteArray<Key>();
        state::ReplicatorEntry replicatorEntry(replicatorKey);

        // Assert:
        AssertValidationResult(
				ValidationResult::Success,
            	replicatorEntry,
				replicatorKey);
    }

    TEST(TEST_CLASS, FailureWhenReplicatorNotRegistered) {
		// Arrange:
        state::ReplicatorEntry replicatorEntry(test::GenerateRandomByteArray<Key>());

        // Assert:
		AssertValidationResult(
				Failure_Storage_Replicator_Not_Registered,
            	replicatorEntry,
				test::GenerateRandomByteArray<Key>());
	}
}}