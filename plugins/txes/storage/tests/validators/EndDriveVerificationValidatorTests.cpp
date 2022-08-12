/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "src/model/InternalStorageNotifications.h"
#include "tests/test/StorageTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS EndDriveVerificationValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(EndDriveVerification,)

    namespace {
        using Notification = model::EndDriveVerificationNotification<1>;

        constexpr auto Current_Height = Height(10);
		constexpr auto Block_Time = 100;	// Unwrapped value of Timestamp
		constexpr auto Shards_Count = 3;
		constexpr auto Replicators_Per_Shards = 4;
		const auto Seed = test::GenerateRandomByteArray<Hash256>();

		state::Verification CreateVerificationWithEmptyShards(const Hash256& trigger, const Timestamp& expiration) {
			return state::Verification{
					trigger,
					expiration,
					test::Random16(),
					state::Shards{}
			};
		}

		void PopulateShards(
				state::BcDriveEntry& driveEntry,
				const uint16_t& shardsCount,
				const uint16_t& replicatorsPerShard,
				const std::map<uint16_t, std::vector<Key>>& requiredShardKeys) {
			auto& verification = driveEntry.verification();
			if (!verification) {
				Hash256 trigger = test::GenerateRandomByteArray<Hash256>();
				Timestamp expiration = test::GenerateRandomValue<Timestamp>();
				verification = CreateVerificationWithEmptyShards(trigger, expiration);
			}

			auto& shards = verification->Shards;
			shards.resize(shardsCount);
			for (auto i = 0u; i < shardsCount; ++i) {
				auto& shard = shards.at(i);
				const auto requiredKeysCount = requiredShardKeys.count(i) ? requiredShardKeys.at(i).size() : 0;

				for (auto j = 0u; j < requiredKeysCount; ++j)
					shard.insert(requiredShardKeys.at(i).at(j));
				for (auto j = 0u; j < replicatorsPerShard - requiredKeysCount; ++j)
					shard.insert(test::GenerateRandomByteArray<Key>());
			}
		}

        void AssertValidationResult(
                ValidationResult expectedResult,
                const state::BcDriveEntry& driveEntry,
                const Key& driveKey,
                const Hash256& trigger,
                const uint16_t shardId,
				const uint16_t keyCount,
				const uint16_t judgingKeyCount,
				const std::vector<Key>& publicKeys) {
            // Arrange:
            auto cache = test::BcDriveCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto& driveCacheDelta = delta.sub<cache::BcDriveCache>();
                driveCacheDelta.insert(driveEntry);
                cache.commit(Current_Height);
            }

			const auto pPublicKeys = std::make_unique<Key[]>(judgingKeyCount);
			for (auto i = 0u; i < publicKeys.size(); ++i)
				pPublicKeys[i] = publicKeys.at(i);

            Notification notification(
                    driveKey,
					Seed,
                    trigger,
					shardId,
					keyCount,
					judgingKeyCount,
					pPublicKeys.get(),
					nullptr,
					nullptr
            );

			auto cacheView = cache.createView();
			auto readOnlyCache = cacheView.toReadOnly();
			auto resolverContext = test::CreateResolverContextXor();
			auto pConfigHolder = config::CreateMockConfigurationHolder();
			auto context = ValidatorContext(pConfigHolder->Config(), Current_Height, Timestamp(Block_Time), resolverContext, readOnlyCache);
			auto pValidator = CreateEndDriveVerificationValidator();

            // Act:
            auto result = test::ValidateNotification(*pValidator, notification, context);

            // Assert:
            EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, FailureWhenDriveNotFound) {
        // Arrange:
        state::BcDriveEntry entry(test::GenerateRandomByteArray<Key>());

        // Assert:
        AssertValidationResult(
                Failure_Storage_Drive_Not_Found,
                entry,
                test::GenerateRandomByteArray<Key>(),
                test::GenerateRandomByteArray<Hash256>(),
                0,
                0,
                0,
				{}
        );
    }

	TEST(TEST_CLASS, FailureVerificationNotSet) {
		// Arrange:
		Key driveKey = test::GenerateRandomByteArray<Key>();
		state::BcDriveEntry entry(driveKey);

		// Assert:
		AssertValidationResult(
				Failure_Storage_Verification_Not_In_Progress,
				entry,
				driveKey,
				test::GenerateRandomByteArray<Hash256>(),
				0,
				0,
				0,
				{}
		);
	}

	TEST(TEST_CLASS, FailureVerificationExpired) {
		// Arrange:
		Key driveKey = test::GenerateRandomByteArray<Key>();
		state::BcDriveEntry entry(driveKey);
		Hash256 trigger = test::GenerateRandomByteArray<Hash256>();
		Timestamp expiration(Block_Time - 10);
		entry.verification() = CreateVerificationWithEmptyShards(trigger, expiration);

		// Assert:
		AssertValidationResult(
				Failure_Storage_Verification_Not_In_Progress,
				entry,
				driveKey,
				test::GenerateRandomByteArray<Hash256>(),
				0,
				0,
				0,
				{}
		);
	}

	TEST(TEST_CLASS, FailureBadVerificationTrigger) {
		// Arrange:
		Key driveKey = test::GenerateRandomByteArray<Key>();
		state::BcDriveEntry entry(driveKey);
		Hash256 trigger = test::GenerateRandomByteArray<Hash256>();
		Timestamp expiration(Block_Time - 10);
		entry.verification() = CreateVerificationWithEmptyShards(trigger, expiration);

		// Assert:
		AssertValidationResult(
				Failure_Storage_Verification_Not_In_Progress,
				entry,
				driveKey,
				test::GenerateRandomByteArray<Hash256>(),
				0,
				0,
				0,
				{}
		);
	}

	TEST(TEST_CLASS, FailureInvalidShardId) {
		// Arrange:
		Key driveKey = test::GenerateRandomByteArray<Key>();
		state::BcDriveEntry entry(driveKey);
		Hash256 trigger = test::GenerateRandomByteArray<Hash256>();
		Timestamp expiration(Block_Time + 10);
		entry.verification() = CreateVerificationWithEmptyShards(trigger, expiration);

		PopulateShards(entry, Shards_Count, Replicators_Per_Shards, {});

		// Assert:
		AssertValidationResult(
				Failure_Storage_Verification_Invalid_Shard_Id,
				entry,
				driveKey,
				trigger,
				Shards_Count,
				0,
				0,
				{}
		);
	}

	TEST(TEST_CLASS, FailureTransactionAlreadyApproved) {
		// Arrange:
		Key driveKey = test::GenerateRandomByteArray<Key>();
		state::BcDriveEntry entry(driveKey);
		Hash256 trigger = test::GenerateRandomByteArray<Hash256>();
		Timestamp expiration(Block_Time + 10);
		entry.verification() = CreateVerificationWithEmptyShards(trigger, expiration);

		PopulateShards(entry, Shards_Count, Replicators_Per_Shards, {});
		const auto targetShardId = test::RandomInRange(0, Shards_Count - 1);
		entry.verification()->Shards.at(targetShardId).clear();

		// Assert:
		AssertValidationResult(
				Failure_Storage_Transaction_Already_Approved,
				entry,
				driveKey,
				trigger,
				targetShardId,
				0,
				0,
				{}
		);
	}

    TEST(TEST_CLASS, FailureInvalidProverCount) {
        // Arrange:
		Key driveKey = test::GenerateRandomByteArray<Key>();
		state::BcDriveEntry entry(driveKey);
		Hash256 trigger = test::GenerateRandomByteArray<Hash256>();
		Timestamp expiration(Block_Time + 10);
		entry.verification() = CreateVerificationWithEmptyShards(trigger, expiration);

		PopulateShards(entry, Shards_Count, Replicators_Per_Shards, {});

        // Assert:
        AssertValidationResult(
                Failure_Storage_Verification_Invalid_Prover_Count,
                entry,
                driveKey,
                trigger,
                test::RandomInRange(0, Shards_Count - 1),
                Replicators_Per_Shards + 2,
                0,
				{}
        );
    }

	TEST(TEST_CLASS, FailureSignatureCountInsufficient) {
		// Arrange:
		Key driveKey = test::GenerateRandomByteArray<Key>();
		state::BcDriveEntry entry(driveKey);
		Hash256 trigger = test::GenerateRandomByteArray<Hash256>();
		Timestamp expiration(Block_Time + 10);
		entry.verification() = CreateVerificationWithEmptyShards(trigger, expiration);

		const auto targetShardId = test::RandomInRange(0, Shards_Count - 1);
		std::vector<Key> judgingKeys = {
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomByteArray<Key>()
		};
		PopulateShards(entry, Shards_Count, Replicators_Per_Shards, {});

		// Assert:
		AssertValidationResult(
				Failure_Storage_Signature_Count_Insufficient,
				entry,
				driveKey,
				trigger,
				targetShardId,
				Replicators_Per_Shards,
				judgingKeys.size(),
				judgingKeys
		);
	}

	TEST(TEST_CLASS, FailureInvalidKey) {
		// Arrange:
		Key driveKey = test::GenerateRandomByteArray<Key>();
		state::BcDriveEntry entry(driveKey);
		Hash256 trigger = test::GenerateRandomByteArray<Hash256>();
		Timestamp expiration(Block_Time + 10);
		entry.verification() = CreateVerificationWithEmptyShards(trigger, expiration);

		const auto targetShardId = test::RandomInRange(0, Shards_Count - 1);
		std::vector<Key> judgingKeys = {
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomByteArray<Key>()
		};
		PopulateShards(entry, Shards_Count, Replicators_Per_Shards, {});

		// Assert:
		AssertValidationResult(
				Failure_Storage_Opinion_Invalid_Key,
				entry,
				driveKey,
				trigger,
				targetShardId,
				Replicators_Per_Shards,
				judgingKeys.size(),
				judgingKeys
		);
	}

    TEST(TEST_CLASS, Success) {
		// Arrange:
		Key driveKey = test::GenerateRandomByteArray<Key>();
		state::BcDriveEntry entry(driveKey);
		Hash256 trigger = test::GenerateRandomByteArray<Hash256>();
		Timestamp expiration(Block_Time + 10);
		entry.verification() = CreateVerificationWithEmptyShards(trigger, expiration);

		const auto targetShardId = test::RandomInRange(0, Shards_Count - 1);
		std::vector<Key> judgingKeys = {
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomByteArray<Key>()
		};
		PopulateShards(entry, Shards_Count, Replicators_Per_Shards, { {targetShardId, judgingKeys} });

		// Assert:
		AssertValidationResult(
				ValidationResult::Success,
				entry,
				driveKey,
				trigger,
				targetShardId,
				Replicators_Per_Shards,
				judgingKeys.size(),
				judgingKeys
		);
    }
}}