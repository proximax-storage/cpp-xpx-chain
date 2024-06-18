/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "catapult/model/StorageNotifications.h"
#include "tests/test/StorageTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS ReplicatorNodeBootKeyValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(ReplicatorNodeBootKey, )

    namespace {
        using Notification = model::ReplicatorNodeBootKeyNotification<1>;

        void AssertValidationResult(ValidationResult expectedResult, const state::BootKeyReplicatorEntry& replicatorEntry, const Key& nodeBootKey) {
            // Arrange:
            auto cache = test::StorageCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto& replicatorCacheDelta = delta.sub<cache::BootKeyReplicatorCache>();
                replicatorCacheDelta.insert(replicatorEntry);
                cache.commit(Height(1));
            }
            Notification notification(test::GenerateRandomByteArray<Key>(), nodeBootKey);
            auto pValidator = CreateReplicatorNodeBootKeyValidator();

            // Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache);

			// Assert:
			EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, FailureWhenBootKeyAlreadyRegisteredWithOtherReplicator) {
        // Arrange:
        Key nodeBootKey = test::GenerateRandomByteArray<Key>();
        auto entry = test::CreateBootKeyReplicatorEntry(nodeBootKey, test::GenerateRandomByteArray<Key>());

        // Assert:
        AssertValidationResult(
			Failure_Storage_Boot_Key_Is_Registered_With_Other_Replicator,
			entry,
			nodeBootKey);
    }

    TEST(TEST_CLASS, Success) {
		// Arrange:
        Key nodeBootKey = test::GenerateRandomByteArray<Key>();
        auto entry = test::CreateBootKeyReplicatorEntry(nodeBootKey, test::GenerateRandomByteArray<Key>());

        // Assert:
		AssertValidationResult(
			ValidationResult::Success,
            entry,
            test::GenerateRandomByteArray<Key>());
	}
}}