/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "plugins/txes/multisig/src/observers/MultisigAccountFacade.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS DriveFilesRewardObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(DriveFilesReward, config::ImmutableConfiguration::Uninitialized())

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::DriveCacheFactory>;
		using Notification = model::DriveFilesRewardNotification<1>;

		constexpr auto Currency_Mosaic_Id = MosaicId(1234);
		constexpr auto Streaming_Mosaic_Id = MosaicId(4321);
		constexpr Height Current_Height(10);
		constexpr Amount Drive_Balance_Streaming(1000);
		constexpr auto Num_Replicators = 10;
		constexpr uint8_t Percent_Approvers = 60;

		auto CreateConfig() {
			auto config = config::ImmutableConfiguration::Uninitialized();
			config.CurrencyMosaicId = Currency_Mosaic_Id;
			config.StreamingMosaicId = Streaming_Mosaic_Id;
			return config;
		}

		state::DriveEntry CreateInitialDriveEntry(
				std::map<Key, state::AccountState>& initialAccounts,
				state::DriveState driveState,
				const Amount& driveBalanceCurrency,
				std::vector<model::UploadInfo>& uploadInfos) {
			state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
			entry.setState(driveState);
			entry.setPercentApprovers(Percent_Approvers);
			auto driveAccount = test::CreateAccount(entry.key());
			driveAccount.Balances.credit(Streaming_Mosaic_Id, Drive_Balance_Streaming);
			driveAccount.Balances.credit(Currency_Mosaic_Id, driveBalanceCurrency);
			initialAccounts.emplace(entry.key(), driveAccount);
			initialAccounts.emplace(entry.owner(), test::CreateAccount(entry.owner()));

			for (auto i = 0u; i < Num_Replicators; ++i) {
				auto key = test::GenerateRandomByteArray<Key>();
				entry.replicators().emplace(key, state::ReplicatorInfo{ Height(i + 1), Height(0), {}, {} });
				initialAccounts.emplace(key, test::CreateAccount(key));
				uploadInfos.push_back(model::UploadInfo{ key, i * 100 });
			}

			return entry;
		}

		state::DriveEntry CreateExpectedDriveEntry(
				state::DriveEntry& initialEntry,
				state::DriveState driveState,
				const std::vector<model::UploadInfo>& uploadInfos,
				std::map<Key, state::AccountState>& expectedAccounts) {
			state::DriveEntry entry(initialEntry);
			entry.setState(driveState);

			auto remainder = Drive_Balance_Streaming.unwrap();
			auto driveBalance = std::floor(double(remainder));
			auto sumUpload = (Num_Replicators - 1) * 100 * Num_Replicators / 2;

			auto lastUploadInfo = uploadInfos.size() - 1;
			for (auto i = 0u; i <= lastUploadInfo; ++i) {
				const auto& uploadInfo = uploadInfos[i];
				auto reward = (lastUploadInfo == i) ? Amount(remainder) : Amount(driveBalance * uploadInfo.Uploaded / sumUpload);
				remainder -= reward.unwrap();
				expectedAccounts.at(entry.key()).Balances.debit(Streaming_Mosaic_Id, reward, Current_Height);
				expectedAccounts.at(uploadInfo.Participant).Balances.credit(Streaming_Mosaic_Id, reward, Current_Height);
				entry.uploadPayments().emplace_back(state::PaymentInformation{ uploadInfo.Participant, reward, Current_Height });
			}

			if (driveState >= state::DriveState::Finished) {
				entry.setEnd(Current_Height);
			}

			return entry;
		}

		struct CacheValues {
		public:
			CacheValues()
				: InitialDriveEntry(Key())
				, ExpectedDriveEntry(Key())
				, InitialMultisigEntry(Key())
				, ExpectedMultisigEntry(Key())
			{}

		public:
			state::DriveEntry InitialDriveEntry;
			state::DriveEntry ExpectedDriveEntry;
			std::map<Key, state::AccountState> InitialAccounts;
			std::map<Key, state::AccountState> ExpectedAccounts;
			std::vector<model::UploadInfo> UploadInfos;
			state::MultisigEntry InitialMultisigEntry;
			state::MultisigEntry ExpectedMultisigEntry;
		};

		void RunTest(NotifyMode mode, const CacheValues& values) {
			// Arrange:
			ObserverTestContext context(mode, Current_Height);
			Notification notification(values.InitialDriveEntry.key(), values.UploadInfos.data(), values.UploadInfos.size());
			auto config = CreateConfig();
			auto pObserver = CreateDriveFilesRewardObserver(config);
			auto& driveCache = context.cache().sub<cache::DriveCache>();
			auto& accountCache = context.cache().sub<cache::AccountStateCache>();
			auto& multisigCache = context.cache().sub<cache::MultisigCache>();

			// Populate cache.
			driveCache.insert(values.InitialDriveEntry);
			for (const auto& initialAccount : values.InitialAccounts)
				accountCache.addAccount(initialAccount.second);

			{
				observers::MultisigAccountFacade facade(multisigCache, values.InitialMultisigEntry.key());
				for (const auto &key : values.InitialMultisigEntry.cosignatories())
					facade.addCosignatory(key);
			}
			if (multisigCache.contains(values.InitialMultisigEntry.key())) {
				auto multisigIter = multisigCache.find(values.InitialMultisigEntry.key());
				auto& multisigEntry = multisigIter.get();
				multisigEntry.setMinApproval(values.InitialMultisigEntry.minApproval());
				multisigEntry.setMinRemoval(values.InitialMultisigEntry.minRemoval());
			}

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			auto driveIter = driveCache.find(values.ExpectedDriveEntry.key());
			const auto& actualEntry = driveIter.get();
			test::AssertEqualDriveData(values.ExpectedDriveEntry, actualEntry);

			for (const auto& expectedAccount : values.ExpectedAccounts) {
				auto accountIter = accountCache.find(expectedAccount.second.Address);
				const auto& actualAccount = accountIter.get();
				test::AssertAccount(expectedAccount.second, actualAccount);
			}

			test::AssertMultisig(multisigCache, values.ExpectedMultisigEntry);
		}
	}

	TEST(TEST_CLASS, DriveFilesReward_Commit_DriveInProgress) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(values.InitialAccounts, state::DriveState::InProgress, Amount(100), values.UploadInfos);
		values.ExpectedAccounts = values.InitialAccounts;
		values.ExpectedDriveEntry = CreateExpectedDriveEntry(values.InitialDriveEntry, state::DriveState::InProgress, values.UploadInfos, values.ExpectedAccounts);
		values.InitialMultisigEntry = test::CreateMultisigEntry(values.InitialDriveEntry);
		values.ExpectedMultisigEntry = values.InitialMultisigEntry;

		// Assert:
		RunTest(NotifyMode::Commit, values);
	}

	TEST(TEST_CLASS, DriveFilesReward_Commit_DriveFinished) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(values.InitialAccounts, state::DriveState::Finished, Amount(100), values.UploadInfos);
		values.ExpectedAccounts = values.InitialAccounts;
		values.ExpectedDriveEntry = CreateExpectedDriveEntry(values.InitialDriveEntry, state::DriveState::Finished, values.UploadInfos, values.ExpectedAccounts);
		values.InitialMultisigEntry = test::CreateMultisigEntry(values.InitialDriveEntry);
		values.ExpectedMultisigEntry = state::MultisigEntry(values.InitialMultisigEntry.key());

		// Assert:
		RunTest(NotifyMode::Commit, values);
	}

	TEST(TEST_CLASS, DriveFilesReward_Rollback_DriveInProgress) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.ExpectedAccounts, state::DriveState::InProgress, Amount(100), values.UploadInfos);
		values.InitialAccounts = values.ExpectedAccounts;
		values.InitialDriveEntry = CreateExpectedDriveEntry(values.ExpectedDriveEntry, state::DriveState::InProgress, values.UploadInfos, values.InitialAccounts);
		values.ExpectedMultisigEntry = test::CreateMultisigEntry(values.ExpectedDriveEntry);
		values.InitialMultisigEntry = values.ExpectedMultisigEntry;

		// Assert:
		RunTest(NotifyMode::Rollback, values);
	}

	TEST(TEST_CLASS, DriveFilesReward_Rollback_DriveFinished) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.ExpectedAccounts, state::DriveState::Finished, Amount(100), values.UploadInfos);
		values.InitialAccounts = values.ExpectedAccounts;
		values.InitialDriveEntry = CreateExpectedDriveEntry(values.ExpectedDriveEntry, state::DriveState::Finished, values.UploadInfos, values.InitialAccounts);
		values.ExpectedMultisigEntry = test::CreateMultisigEntry(values.ExpectedDriveEntry);
		values.InitialMultisigEntry = state::MultisigEntry(values.ExpectedMultisigEntry.key());

		// Assert:
		RunTest(NotifyMode::Rollback, values);
	}

	TEST(TEST_CLASS, DriveFilesReward_Rollback_InvalidParticipantPayment_WrongKey) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.ExpectedAccounts, state::DriveState::InProgress, Amount(100), values.UploadInfos);
		values.InitialAccounts = values.ExpectedAccounts;
		values.InitialDriveEntry = CreateExpectedDriveEntry(values.ExpectedDriveEntry, state::DriveState::InProgress, values.UploadInfos, values.InitialAccounts);
		values.InitialDriveEntry.uploadPayments().back().Receiver = test::GenerateRandomByteArray<Key>();

		// Assert:
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values), catapult_runtime_error);
	}

	TEST(TEST_CLASS, DriveFilesReward_Rollback_InvalidParticipantPayment_WrongHeight) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.ExpectedAccounts, state::DriveState::InProgress, Amount(100), values.UploadInfos);
		values.InitialAccounts = values.ExpectedAccounts;
		values.InitialDriveEntry = CreateExpectedDriveEntry(values.ExpectedDriveEntry, state::DriveState::InProgress, values.UploadInfos, values.InitialAccounts);
		values.InitialDriveEntry.uploadPayments().back().Height = Current_Height + Height(1);

		// Assert:
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values), catapult_runtime_error);
	}
}}
