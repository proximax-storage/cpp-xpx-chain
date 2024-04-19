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

    DEFINE_COMMON_VALIDATOR_TESTS(ReplicatorsCleanup, )

    namespace {
        using Notification = model::ReplicatorsCleanupNotification<1>;

		auto CreateConfig(model::NetworkIdentifier networkIdentifier) {
			test::MutableBlockchainConfiguration config;
			config.Immutable.NetworkIdentifier = networkIdentifier;
			return config.ToConst();
		}

        void AssertValidationResult(ValidationResult expectedResult, model::NetworkIdentifier networkIdentifier, const std::vector<std::pair<Key, Key>>& keysToInsert, const std::vector<Key>& keysToRemove) {
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
            Notification notification(keysToRemove.size(), keysToRemove.data());
            auto pValidator = CreateReplicatorsCleanupValidator();

            // Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, CreateConfig(networkIdentifier));

			// Assert:
			EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, FailureWhenNetworkIsPublic) {
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
			Key({ 4 }),
		};

        // Assert:
        AssertValidationResult(
			Failure_Storage_Replicator_Cleanup_Is_Unallowed_In_Public_Network,
			model::NetworkIdentifier::Public,
			keysToInsert,
			keysToRemove);
    }

    TEST(TEST_CLASS, FailureWhenReplicatorNotFound) {
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
        AssertValidationResult(
			Failure_Storage_Replicator_Not_Found,
			model::NetworkIdentifier::Mijin_Test,
			keysToInsert,
			keysToRemove);
    }

    TEST(TEST_CLASS, FailureWhenReplicatorIsBoundWithBootKey) {
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
        AssertValidationResult(
			Failure_Storage_Replicator_Is_Bound_With_Boot_Key,
			model::NetworkIdentifier::Mijin_Test,
			keysToInsert,
			keysToRemove);
    }

    TEST(TEST_CLASS, Success) {
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
        AssertValidationResult(
			ValidationResult::Success,
			model::NetworkIdentifier::Mijin_Test,
			keysToInsert,
			keysToRemove);
	}
}}