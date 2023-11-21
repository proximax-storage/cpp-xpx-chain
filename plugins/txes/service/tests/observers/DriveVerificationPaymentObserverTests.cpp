/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS DriveVerificationPaymentObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(DriveVerificationPayment, MosaicId())

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::DriveCacheFactory>;
		using Notification = model::DriveVerificationPaymentNotification<1>;

		constexpr auto Storage_Mosaic_Id = MosaicId(1234);
		constexpr Height Current_Height(20);
		constexpr Height Billing_Start(5);
		constexpr Height Billing_End(30);
		constexpr Amount Drive_Balance(1000);
		constexpr auto Drive_Size = 100;
		constexpr auto Num_Replicators = 10;
		constexpr uint8_t Percent_Approvers(60);

		state::MultisigEntry CreateMultisigEntry(const Key& key, uint8_t minApproval, uint8_t minRemoval) {
			state::MultisigEntry entry(key);
			entry.setMinApproval(minApproval);
			entry.setMinRemoval(minRemoval);

			return entry;
		}

		state::DriveEntry CreateInitialDriveEntry(std::map<Key, state::AccountState>& initialAccounts) {
			state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
			entry.setSize(Drive_Size);
			entry.setPercentApprovers(Percent_Approvers);
			auto driveAccount = test::CreateAccount(entry.key());
			driveAccount.Balances.credit(Storage_Mosaic_Id, Drive_Balance);
			initialAccounts.emplace(entry.key(), driveAccount);

			entry.billingHistory().push_back(state::BillingPeriodDescription{ Billing_Start, Billing_End, {} });

			for (auto i = 0u; i < Num_Replicators; ++i) {
				auto key = test::GenerateRandomByteArray<Key>();
				entry.replicators().emplace(key, state::ReplicatorInfo{ Billing_Start + Height(i + 1), Height(0), {}, {} });
				initialAccounts.emplace(key, test::CreateAccount(key));
			}

			return entry;
		}

		state::DriveEntry CreateExpectedDriveEntry(
				state::DriveEntry& initialEntry,
				uint8_t numFailures,
				std::vector<Key>& failedReplicators,
				std::map<Key, state::AccountState>& expectedAccounts) {
			state::DriveEntry entry(initialEntry);

			expectedAccounts.at(entry.key()).Balances.credit(Storage_Mosaic_Id, Amount(Drive_Size * numFailures), Current_Height);
			auto driveBalance = std::floor(double(Drive_Balance.unwrap() + Drive_Size * numFailures));

			auto firstReplicatorStart = (Billing_End - Billing_Start).unwrap() - 1;
			auto lastReplicatorStart = firstReplicatorStart - (Num_Replicators - 1);
			auto sumTime = (firstReplicatorStart + lastReplicatorStart) * Num_Replicators / 2 - numFailures * (Billing_End - Current_Height).unwrap();

			failedReplicators.reserve(numFailures);
			auto& replicators = entry.replicators();
			auto replicatorIter = replicators.begin();
			for (auto i = 0u; i < numFailures && replicatorIter != replicators.end(); ++i) {
				auto replicatorKey = replicatorIter->first;
				failedReplicators.push_back(replicatorKey);
				replicatorIter->second.End = Current_Height;
				auto replicatorReward = Amount(driveBalance * (Current_Height - replicatorIter->second.Start).unwrap() / sumTime);
				expectedAccounts.at(entry.key()).Balances.debit(Storage_Mosaic_Id, replicatorReward, Current_Height);
				expectedAccounts.at(replicatorKey).Balances.credit(Storage_Mosaic_Id, replicatorReward, Current_Height);
				entry.billingHistory().back().Payments.emplace_back(state::PaymentInformation{ replicatorKey, Amount(replicatorReward), Current_Height });
				++replicatorIter;
				entry.removeReplicator(replicatorKey);
			}

			return entry;
		}

		struct CacheValues {
		public:
			CacheValues() : InitialDriveEntry(Key()), ExpectedDriveEntry(Key()), InitialMultisigEntry(Key()), ExpectedMultisigEntry(Key())
			{}

		public:
			state::DriveEntry InitialDriveEntry;
			state::DriveEntry ExpectedDriveEntry;
			std::vector<Key> FailedReplicators;
			std::map<Key, state::AccountState> InitialAccounts;
			std::map<Key, state::AccountState> ExpectedAccounts;
			state::MultisigEntry InitialMultisigEntry;
			state::MultisigEntry ExpectedMultisigEntry;
		};

		void RunTest(NotifyMode mode, const CacheValues& values) {
			// Arrange:
			ObserverTestContext context(mode, Current_Height);
			Notification notification(
				values.InitialDriveEntry.key(),
				values.FailedReplicators.size(),
				values.FailedReplicators.data());
			auto pObserver = CreateDriveVerificationPaymentObserver(Storage_Mosaic_Id);
			auto& driveCache = context.cache().sub<cache::DriveCache>();
			auto& multisigCache = context.cache().sub<cache::MultisigCache>();
			auto& accountCache = context.cache().sub<cache::AccountStateCache>();

			// Populate cache.
			driveCache.insert(values.InitialDriveEntry);
			multisigCache.insert(values.InitialMultisigEntry);
			for (const auto& initialAccount : values.InitialAccounts) {
				accountCache.addAccount(initialAccount.second);
			}

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			auto driveIter = driveCache.find(values.ExpectedDriveEntry.key());
			const auto& actualEntry = driveIter.get();
			test::AssertEqualDriveData(values.ExpectedDriveEntry, actualEntry);

			auto multisigIter = multisigCache.find(values.ExpectedMultisigEntry.key());
			const auto& actualMultisigEntry = multisigIter.get();
			EXPECT_EQ(values.ExpectedMultisigEntry.minApproval(), actualMultisigEntry.minApproval());
			EXPECT_EQ(values.ExpectedMultisigEntry.minRemoval(), actualMultisigEntry.minRemoval());

			for (const auto& expectedAccount : values.ExpectedAccounts) {
				auto accountIter = accountCache.find(expectedAccount.second.Address);
				const auto& actualAccount = accountIter.get();
				test::AssertAccount(expectedAccount.second, actualAccount);
			}
		}
	}

	TEST(TEST_CLASS, DriveVerificationPayment_Commit_NoFailures) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(values.InitialAccounts);
		values.ExpectedAccounts = values.InitialAccounts;
		values.ExpectedDriveEntry = CreateExpectedDriveEntry(values.InitialDriveEntry, 0, values.FailedReplicators, values.ExpectedAccounts);
		values.InitialMultisigEntry = CreateMultisigEntry(values.InitialDriveEntry.key(), 6, 6);
		values.ExpectedMultisigEntry = CreateMultisigEntry(values.InitialDriveEntry.key(), 6, 6);

		// Assert:
		RunTest(NotifyMode::Commit, values);
	}

	TEST(TEST_CLASS, DriveVerificationPayment_Commit) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(values.InitialAccounts);
		values.ExpectedAccounts = values.InitialAccounts;
		values.ExpectedDriveEntry = CreateExpectedDriveEntry(values.InitialDriveEntry, 3, values.FailedReplicators, values.ExpectedAccounts);
		values.InitialMultisigEntry = CreateMultisigEntry(values.InitialDriveEntry.key(), 6, 6);
		values.ExpectedMultisigEntry = CreateMultisigEntry(values.InitialDriveEntry.key(), 5, 5);

		// Assert:
		RunTest(NotifyMode::Commit, values);
	}

	TEST(TEST_CLASS, DriveVerificationPayment_Rollback_NoFailures) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.ExpectedAccounts);
		values.InitialAccounts = values.ExpectedAccounts;
		values.InitialDriveEntry = CreateExpectedDriveEntry(values.ExpectedDriveEntry, 0, values.FailedReplicators, values.InitialAccounts);
		values.InitialMultisigEntry = CreateMultisigEntry(values.InitialDriveEntry.key(), 5, 5);
		values.ExpectedMultisigEntry = CreateMultisigEntry(values.InitialDriveEntry.key(), 5, 5);

		// Assert:
		RunTest(NotifyMode::Rollback, values);
	}

	TEST(TEST_CLASS, DriveVerificationPayment_Rollback) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.ExpectedAccounts);
		values.InitialAccounts = values.ExpectedAccounts;
		values.InitialDriveEntry = CreateExpectedDriveEntry(values.ExpectedDriveEntry, 3, values.FailedReplicators, values.InitialAccounts);
		values.InitialMultisigEntry = CreateMultisigEntry(values.InitialDriveEntry.key(), 5, 5);
		values.ExpectedMultisigEntry = CreateMultisigEntry(values.InitialDriveEntry.key(), 6, 6);

		// Assert:
		RunTest(NotifyMode::Rollback, values);
	}
}}
