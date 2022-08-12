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

#define TEST_CLASS EndDriveObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(EndDrive, config::ImmutableConfiguration::Uninitialized())

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::DriveCacheFactory>;
		using Notification = model::EndDriveNotification<1>;

		constexpr auto Currency_Mosaic_Id = MosaicId(5678);
		constexpr auto Storage_Mosaic_Id = MosaicId(1234);
		constexpr auto Streaming_Mosaic_Id = MosaicId(4321);
		constexpr Height Billing_Start(5);
		constexpr Height Billing_End(20);
		constexpr Amount Drive_Balance_Currency(100);
		constexpr Amount Drive_Balance_Storage(1000);
		constexpr auto Drive_Size = 100;
		constexpr auto Num_Replicators = 10;
		constexpr auto Num_Files = 20;
		constexpr uint8_t Percent_Approvers = 60;

		auto CreateConfig() {
			auto config = config::ImmutableConfiguration::Uninitialized();
			config.CurrencyMosaicId = Currency_Mosaic_Id;
			config.StorageMosaicId = Storage_Mosaic_Id;
			config.StreamingMosaicId = Streaming_Mosaic_Id;
			return config;
		}

		void AddFiles(state::DriveEntry& entry) {
			for (auto i = 0u; i < Num_Files; ++i) {
				auto fileHash = test::GenerateRandomByteArray<Hash256>();
				uint64_t fileSize = (i + 1) * 100;
				entry.files().emplace(fileHash, state::FileInfo{ fileSize });
				for (auto& replicator : entry.replicators())
					replicator.second.ActiveFilesWithoutDeposit.insert(fileHash);
			}
		}

		state::DriveEntry CreateInitialDriveEntry(
				std::map<Key, state::AccountState>& initialAccounts,
				state::DriveState driveState,
				const Amount& driveBalanceStreaming) {
			state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
			entry.setSize(Drive_Size);
			entry.setState(driveState);
			entry.setBillingPeriod(BlockDuration((Billing_End - Billing_Start).unwrap()));
			entry.setPercentApprovers(Percent_Approvers);
			auto driveAccount = test::CreateAccount(entry.key());
			driveAccount.Balances.credit(Currency_Mosaic_Id, Drive_Balance_Currency);
			driveAccount.Balances.credit(Storage_Mosaic_Id, Drive_Balance_Storage);
			driveAccount.Balances.credit(Streaming_Mosaic_Id, driveBalanceStreaming);
			initialAccounts.emplace(entry.key(), driveAccount);
			initialAccounts.emplace(entry.owner(), test::CreateAccount(entry.owner()));

			entry.billingHistory().push_back(state::BillingPeriodDescription{ Billing_Start, Billing_End, {} });

			for (auto i = 0u; i < Num_Replicators; ++i) {
				auto key = test::GenerateRandomByteArray<Key>();
				entry.replicators().emplace(key, state::ReplicatorInfo{ Billing_Start + Height(i + 1), Height(0), {}, {} });
				initialAccounts.emplace(key, test::CreateAccount(key));
			}

			AddFiles(entry);

			return entry;
		}

		state::DriveEntry CreateExpectedDriveEntry(
				state::DriveEntry& initialEntry,
				std::map<Key, state::AccountState>& expectedAccounts,
				const Height& height) {
			state::DriveEntry entry(initialEntry);
			entry.billingHistory().back().End = height;
			entry.setState(state::DriveState::Finished);
			auto& driveAccount = expectedAccounts.at(entry.key());

			auto remainder = Drive_Balance_Storage.unwrap();
			auto driveBalance = std::floor(double(remainder));
			auto firstReplicatorStart = (height - Billing_Start).unwrap() - 1;
			auto lastReplicatorStart = firstReplicatorStart - (Num_Replicators - 1);
			auto sumTime = (firstReplicatorStart + lastReplicatorStart) * Num_Replicators / 2;

			auto& replicators = entry.replicators();
			auto lastReplicatorIter = --replicators.end();
			for (auto replicatorIter = replicators.begin(); replicatorIter != replicators.end(); ++replicatorIter) {
				auto replicatorKey = replicatorIter->first;
				auto replicatorReward = (replicatorIter == lastReplicatorIter) ? Amount(remainder) :
					Amount(driveBalance * (height - replicatorIter->second.Start).unwrap() / sumTime);
				remainder -= replicatorReward.unwrap();
				driveAccount.Balances.debit(Storage_Mosaic_Id, replicatorReward, height);
				auto& replicatorAccount = expectedAccounts.at(replicatorKey);
				replicatorAccount.Balances.credit(Storage_Mosaic_Id, replicatorReward + Amount(Drive_Size), height);
				entry.billingHistory().back().Payments.emplace_back(state::PaymentInformation{ replicatorKey, replicatorReward, height });

				auto r = 0u;
				for (const auto& filePair : entry.files()) {
					replicatorIter->second.ActiveFilesWithoutDeposit.erase(filePair.first);
					if (r++ % 2) {
						replicatorIter->second.AddInactiveUndepositedFile(filePair.first, height);
					} else {
						initialEntry.replicators().at(replicatorIter->first).ActiveFilesWithoutDeposit.erase(filePair.first);
						replicatorAccount.Balances.credit(Streaming_Mosaic_Id, Amount(filePair.second.Size), height);
					}
				}
			}

			auto remainingCurrency = driveAccount.Balances.get(Currency_Mosaic_Id);
			entry.uploadPayments().emplace_back(state::PaymentInformation{ entry.owner(), remainingCurrency, height });
			driveAccount.Balances.debit(Currency_Mosaic_Id, remainingCurrency, height);
			expectedAccounts.at(entry.owner()).Balances.credit(Currency_Mosaic_Id, remainingCurrency, height);

			if (driveAccount.Balances.get(Streaming_Mosaic_Id) == Amount(0)) {
				entry.setEnd(height);
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
			state::MultisigEntry InitialMultisigEntry;
			state::MultisigEntry ExpectedMultisigEntry;
		};

		void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight) {
			// Arrange:
			ObserverTestContext context(mode, currentHeight);
			Notification notification(values.InitialDriveEntry.key(), test::GenerateRandomByteArray<Key>());
			auto config = CreateConfig();
			auto pObserver = CreateEndDriveObserver(config);
			auto& driveCache = context.cache().sub<cache::DriveCache>();
			auto& accountCache = context.cache().sub<cache::AccountStateCache>();
			auto& multisigCache = context.cache().sub<cache::MultisigCache>();

			// Populate cache.
			driveCache.insert(values.InitialDriveEntry);
			driveCache.addBillingDrive(values.InitialDriveEntry.key(), Billing_End);
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

	TEST(TEST_CLASS, EndDrive_Commit_DriveEndedAtBillingPeriodEnd_NoStreamingUnits) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(values.InitialAccounts, state::DriveState::Pending, Amount(0));
		values.ExpectedAccounts = values.InitialAccounts;
		values.ExpectedDriveEntry = CreateExpectedDriveEntry(values.InitialDriveEntry, values.ExpectedAccounts, Billing_End);
		values.InitialMultisigEntry = test::CreateMultisigEntry(values.InitialDriveEntry);
		values.ExpectedMultisigEntry = state::MultisigEntry(values.InitialMultisigEntry.key());

		// Assert:
		RunTest(NotifyMode::Commit, values, Billing_End);
	}

	TEST(TEST_CLASS, EndDrive_Commit_DriveEndedAtBillingPeriodEnd_WithStreamingUnits) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(values.InitialAccounts, state::DriveState::Pending, Amount(10));
		values.ExpectedAccounts = values.InitialAccounts;
		values.ExpectedDriveEntry = CreateExpectedDriveEntry(values.InitialDriveEntry, values.ExpectedAccounts, Billing_End);
		values.InitialMultisigEntry = test::CreateMultisigEntry(values.InitialDriveEntry);
		values.ExpectedMultisigEntry = values.InitialMultisigEntry;

		// Assert:
		RunTest(NotifyMode::Commit, values, Billing_End);
	}

	TEST(TEST_CLASS, EndDrive_Commit_DriveEndedBeforeBillingPeriodEnd_NoStreamingUnits) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(values.InitialAccounts, state::DriveState::InProgress, Amount(0));
		values.ExpectedAccounts = values.InitialAccounts;
		values.ExpectedDriveEntry = CreateExpectedDriveEntry(values.InitialDriveEntry, values.ExpectedAccounts, Billing_End - Height(1));
		values.InitialMultisigEntry = test::CreateMultisigEntry(values.InitialDriveEntry);
		values.ExpectedMultisigEntry = state::MultisigEntry(values.InitialMultisigEntry.key());

		// Assert:
		RunTest(NotifyMode::Commit, values, Billing_End - Height(1));
	}

	TEST(TEST_CLASS, EndDrive_Commit_DriveEndedBeforeBillingPeriodEnd_WithStreamingUnits) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(values.InitialAccounts, state::DriveState::InProgress, Amount(10));
		values.ExpectedAccounts = values.InitialAccounts;
		values.ExpectedDriveEntry = CreateExpectedDriveEntry(values.InitialDriveEntry, values.ExpectedAccounts, Billing_End - Height(1));
		values.InitialMultisigEntry = test::CreateMultisigEntry(values.InitialDriveEntry);
		values.ExpectedMultisigEntry = values.InitialMultisigEntry;

		// Assert:
		RunTest(NotifyMode::Commit, values, Billing_End - Height(1));
	}

	TEST(TEST_CLASS, EndDrive_Rollback_DriveEndedAtBillingPeriodEnd_NoStreamingUnits) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.ExpectedAccounts, state::DriveState::Pending, Amount(0));
		values.InitialAccounts = values.ExpectedAccounts;
		values.InitialDriveEntry = CreateExpectedDriveEntry(values.ExpectedDriveEntry, values.InitialAccounts, Billing_End);
		values.ExpectedMultisigEntry = test::CreateMultisigEntry(values.ExpectedDriveEntry);
		values.InitialMultisigEntry = values.ExpectedMultisigEntry;

		// Assert:
		RunTest(NotifyMode::Rollback, values, Billing_End);
	}

	TEST(TEST_CLASS, EndDrive_Rollback_DriveEndedAtBillingPeriodEnd_WithStreamingUnits) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.ExpectedAccounts, state::DriveState::Pending, Amount(10));
		values.InitialAccounts = values.ExpectedAccounts;
		values.InitialDriveEntry = CreateExpectedDriveEntry(values.ExpectedDriveEntry, values.InitialAccounts, Billing_End);
		values.ExpectedMultisigEntry = test::CreateMultisigEntry(values.ExpectedDriveEntry);
		values.InitialMultisigEntry = values.ExpectedMultisigEntry;

		// Assert:
		RunTest(NotifyMode::Rollback, values, Billing_End);
	}

	TEST(TEST_CLASS, EndDrive_Rollback_DriveEndedBeforeBillingPeriodEnd_NoStreamingUnits) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.ExpectedAccounts, state::DriveState::InProgress, Amount(0));
		values.InitialAccounts = values.ExpectedAccounts;
		values.InitialDriveEntry = CreateExpectedDriveEntry(values.ExpectedDriveEntry, values.InitialAccounts, Billing_End - Height(1));
		values.ExpectedMultisigEntry = test::CreateMultisigEntry(values.ExpectedDriveEntry);
		values.InitialMultisigEntry = values.ExpectedMultisigEntry;

		// Assert:
		RunTest(NotifyMode::Rollback, values, Billing_End - Height(1));
	}

	TEST(TEST_CLASS, EndDrive_Rollback_DriveEndedBeforeBillingPeriodEnd_WithStreamingUnits) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.ExpectedAccounts, state::DriveState::InProgress, Amount(10));
		values.InitialAccounts = values.ExpectedAccounts;
		values.InitialDriveEntry = CreateExpectedDriveEntry(values.ExpectedDriveEntry, values.InitialAccounts, Billing_End - Height(1));
		values.ExpectedMultisigEntry = test::CreateMultisigEntry(values.ExpectedDriveEntry);
		values.InitialMultisigEntry = values.ExpectedMultisigEntry;

		// Assert:
		RunTest(NotifyMode::Rollback, values, Billing_End - Height(1));
	}
}}
