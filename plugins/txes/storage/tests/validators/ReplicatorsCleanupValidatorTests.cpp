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

#define TEST_CLASS ReplicatorsCleanupValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(ReplicatorsCleanupV1, )

    namespace {
		template<typename TNotification>
        void AssertValidationResult(
				ValidationResult expectedResult,
				const std::vector<std::pair<Key, Key>>& keysToInsert,
				const std::vector<Key>& keysToRemove,
				std::unique_ptr<const stateful::NotificationValidatorT<TNotification>> pValidator) {
            // Arrange:
            auto cache = test::StorageCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto& replicatorCacheDelta = delta.sub<cache::ReplicatorCache>();
				for (const auto& [ replicatorKey, nodeBootKey ] : keysToInsert) {
					state::ReplicatorEntry entry(replicatorKey);
					entry.setNodeBootKey(nodeBootKey);
					replicatorCacheDelta.insert(entry);
				}
                cache.commit(Height(1));
            }
			TNotification notification(keysToRemove.size(), keysToRemove.data());

            // Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache);

			// Assert:
			EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, FailureWhenNoReplicatorsToRemoveV1) {
        // Arrange:
		std::vector<std::pair<Key, Key>> keysToInsert{
			{ Key({ 1 }), Key({ 5 }) },
			{ Key({ 2 }), Key() },
			{ Key({ 3 }), Key() },
			{ Key({ 4 }), Key() },
		};
		std::vector<Key> keysToRemove{};

        // Assert:
        AssertValidationResult<model::ReplicatorsCleanupNotification<1>>(
			Failure_Storage_No_Replicators_To_Remove,
			keysToInsert,
			keysToRemove,
			CreateReplicatorsCleanupV1Validator());
    }

    TEST(TEST_CLASS, FailureWhenNoReplicatorsToRemoveV2) {
        // Arrange:
		std::vector<std::pair<Key, Key>> keysToInsert{
			{ Key({ 1 }), Key({ 5 }) },
			{ Key({ 2 }), Key() },
			{ Key({ 3 }), Key() },
			{ Key({ 4 }), Key() },
		};
		std::vector<Key> keysToRemove{};

        // Assert:
        AssertValidationResult<model::ReplicatorsCleanupNotification<2>>(
			Failure_Storage_No_Replicators_To_Remove,
			keysToInsert,
			keysToRemove,
			CreateReplicatorsCleanupV2Validator());
    }

    TEST(TEST_CLASS, FailureWhenReplicatorNotFoundV1) {
        // Arrange:
		std::vector<std::pair<Key, Key>> keysToInsert{
			{ Key({ 1 }), Key({ 5 }) },
			{ Key({ 2 }), Key() },
			{ Key({ 3 }), Key() },
			{ Key({ 4 }), Key() },
		};
		std::vector<Key> keysToRemove{
			Key({ 2 }),
			Key({ 3 }),
			Key({ 4 }),
			Key({ 7 }),
			Key({ 1 }),
		};

        // Assert:
        AssertValidationResult<model::ReplicatorsCleanupNotification<1>>(
			Failure_Storage_Replicator_Not_Found,
			keysToInsert,
			keysToRemove,
			CreateReplicatorsCleanupV1Validator());
    }

    TEST(TEST_CLASS, FailureWhenReplicatorNotFoundV2) {
        // Arrange:
		std::vector<std::pair<Key, Key>> keysToInsert{
			{ Key({ 1 }), Key({ 5 }) },
			{ Key({ 2 }), Key() },
			{ Key({ 3 }), Key() },
			{ Key({ 4 }), Key() },
		};
		std::vector<Key> keysToRemove{
			Key({ 2 }),
			Key({ 3 }),
			Key({ 4 }),
			Key({ 7 }),
			Key({ 1 }),
		};

        // Assert:
        AssertValidationResult<model::ReplicatorsCleanupNotification<2>>(
			Failure_Storage_Replicator_Not_Found,
			keysToInsert,
			keysToRemove,
			CreateReplicatorsCleanupV2Validator());
    }

    TEST(TEST_CLASS, FailureWhenReplicatorIsBoundWithBootKeyV1) {
        // Arrange:
		std::vector<std::pair<Key, Key>> keysToInsert{
			{ Key({ 1 }), Key({ 5 }) },
			{ Key({ 2 }), Key() },
			{ Key({ 3 }), Key() },
			{ Key({ 4 }), Key() },
		};
		std::vector<Key> keysToRemove{
			Key({ 1 }),
			Key({ 2 }),
			Key({ 3 }),
		};

        // Assert:
        AssertValidationResult<model::ReplicatorsCleanupNotification<1>>(
			Failure_Storage_Replicator_Is_Bound_With_Boot_Key,
			keysToInsert,
			keysToRemove,
			CreateReplicatorsCleanupV1Validator());
    }

    TEST(TEST_CLASS, SuccessV1) {
        // Arrange:
		std::vector<std::pair<Key, Key>> keysToInsert{
			{ Key({ 1 }), Key({ 5 }) },
			{ Key({ 2 }), Key() },
			{ Key({ 3 }), Key() },
			{ Key({ 4 }), Key() },
		};
		std::vector<Key> keysToRemove{
			Key({ 2 }),
			Key({ 3 }),
			Key({ 4 }),
		};

        // Assert:
        AssertValidationResult<model::ReplicatorsCleanupNotification<1>>(
			ValidationResult::Success,
			keysToInsert,
			keysToRemove,
			CreateReplicatorsCleanupV1Validator());
	}

    TEST(TEST_CLASS, SuccessV2) {
        // Arrange:
		std::vector<std::pair<Key, Key>> keysToInsert{
			{ Key({ 1 }), Key({ 5 }) },
			{ Key({ 2 }), Key() },
			{ Key({ 3 }), Key() },
			{ Key({ 4 }), Key() },
		};
		std::vector<Key> keysToRemove{
			Key({ 2 }),
			Key({ 3 }),
			Key({ 4 }),
		};

        // Assert:
        AssertValidationResult<model::ReplicatorsCleanupNotification<2>>(
			ValidationResult::Success,
			keysToInsert,
			keysToRemove,
			CreateReplicatorsCleanupV2Validator());
	}
}}