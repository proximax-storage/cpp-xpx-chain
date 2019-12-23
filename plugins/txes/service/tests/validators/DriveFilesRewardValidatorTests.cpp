/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache_core/AccountStateCache.h"
#include "src/cache/DriveCache.h"
#include "src/validators/Validators.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS DriveFilesRewardValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(DriveFilesReward, MosaicId())

	namespace {
		using Notification = model::DriveFilesRewardNotification<1>;

		constexpr auto Streaming_Mosaic_Id = MosaicId(1234);

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::DriveEntry& entry,
				const std::vector<model::UploadInfo>& uploadInfos = {},
				const Amount& driveBalance = Amount(100)) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::DriveCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& driveCacheDelta = delta.sub<cache::DriveCache>();
				driveCacheDelta.insert(entry);

				auto& accountStateCacheDelta = delta.sub<cache::AccountStateCache>();
				auto driveAccount = test::CreateAccount(entry.key());
				driveAccount.Balances.credit(Streaming_Mosaic_Id, driveBalance);
				accountStateCacheDelta.addAccount(driveAccount);

				cache.commit(currentHeight);
			}
			Notification notification(entry.key(), uploadInfos.data(), uploadInfos.size());
			auto pValidator = CreateDriveFilesRewardValidator(Streaming_Mosaic_Id);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenNoUploadInfo) {
		// Assert:
		AssertValidationResult(
			Failure_Service_Zero_Infos,
			state::DriveEntry(test::GenerateRandomByteArray<Key>()));
	}

	TEST(TEST_CLASS, FailureWhenInNotStartedState) {
		// Arrange:
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setState(state::DriveState::NotStarted);

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Not_In_Pending_State,
			entry,
			{ { test::GenerateRandomByteArray<Key>(), 10 } });
	}

	TEST(TEST_CLASS, FailureWhenInInProgressState) {
		// Arrange:
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setState(state::DriveState::InProgress);

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Not_In_Pending_State,
			entry,
			{ { test::GenerateRandomByteArray<Key>(), 10 } });
	}

	TEST(TEST_CLASS, FailureWhenNoStreamingTokens) {
		// Arrange:
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setState(state::DriveState::Pending);

		// Assert:
		AssertValidationResult(
			Failure_Service_Doesnt_Contain_Streaming_Tokens,
			entry,
			{ { test::GenerateRandomByteArray<Key>(), 10 } },
			Amount(0));
	}

	TEST(TEST_CLASS, FailureWhenZeroUploadInfo) {
		// Arrange:
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setState(state::DriveState::Pending);

		// Assert:
		AssertValidationResult(
			Failure_Service_Zero_Upload_Info,
			entry,
			{ { test::GenerateRandomByteArray<Key>(), 0 } });
	}

	TEST(TEST_CLASS, FailureWhenParticipantIsNotRegisteredToDrive) {
		// Arrange:
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setState(state::DriveState::Pending);

		// Assert:
		AssertValidationResult(
			Failure_Service_Participant_Is_Not_Registered_To_Drive,
			entry,
			{ { test::GenerateRandomByteArray<Key>(), 10 } });
	}

	TEST(TEST_CLASS, FailureWhenReplicatorHasActiveFileWithoutDeposit) {
		// Arrange:
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setState(state::DriveState::Pending);
		auto replicatorKey = test::GenerateRandomByteArray<Key>();
		state::ReplicatorInfo replicatorInfo{ Height(1), Height(0), { test::GenerateRandomByteArray<Hash256>() }, {} };
		entry.replicators().emplace(replicatorKey, replicatorInfo);

		// Assert:
		AssertValidationResult(
			Failure_Service_Replicator_Has_Active_File_Without_Deposit,
			entry,
			{ { replicatorKey, 10 } });
	}

	TEST(TEST_CLASS, FailureWhenParticipantRedundant) {
		// Arrange:
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setState(state::DriveState::Pending);
		auto replicatorKey = test::GenerateRandomByteArray<Key>();
		state::ReplicatorInfo replicatorInfo{ Height(1), Height(0), {}, {} };
		entry.replicators().emplace(replicatorKey, replicatorInfo);

		// Assert:
		AssertValidationResult(
			Failure_Service_Participant_Redundant,
			entry,
			{ { replicatorKey, 10 }, { replicatorKey, 20 } });
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setState(state::DriveState::Pending);
		auto replicatorKey = test::GenerateRandomByteArray<Key>();
		state::ReplicatorInfo replicatorInfo{ Height(1), Height(0), {}, {} };
		entry.replicators().emplace(replicatorKey, replicatorInfo);

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			entry,
			{ { replicatorKey, 10 } });
	}
}}
