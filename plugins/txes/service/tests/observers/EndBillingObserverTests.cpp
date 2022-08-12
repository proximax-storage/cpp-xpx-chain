/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/ServiceTestUtils.h"
#include "tests/test/core/NotificationTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS EndBillingObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(EndBilling, MosaicId())

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::DriveCacheFactory>;

		constexpr auto Storage_Mosaic_Id = MosaicId(1234);
		constexpr Height Billing_Start(5);
		constexpr Height Billing_End(20);
		constexpr Amount Drive_Balance(1000);
		constexpr auto Drive_Size = 100;
		constexpr auto Num_Replicators = 10;

		state::DriveEntry CreateInitialDriveEntry(std::map<Key, state::AccountState>& initialAccounts, state::DriveState driveState) {
			state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
			entry.setSize(Drive_Size);
			entry.setState(driveState);
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
				std::map<Key, state::AccountState>& expectedAccounts) {
			state::DriveEntry entry(initialEntry);
			entry.setState(state::DriveState::Pending);

			auto remainder = Drive_Balance.unwrap();
			auto driveBalance = std::floor(double(remainder));
			auto firstReplicatorStart = (Billing_End - Billing_Start).unwrap() - 1;
			auto lastReplicatorStart = firstReplicatorStart - (Num_Replicators - 1);
			auto sumTime = (firstReplicatorStart + lastReplicatorStart) * Num_Replicators / 2;

			auto& replicators = entry.replicators();
			auto lastReplicatorIter = --replicators.end();
			for (auto replicatorIter = replicators.begin(); replicatorIter != replicators.end(); ++replicatorIter) {
				auto replicatorKey = replicatorIter->first;
				auto replicatorReward = (replicatorIter == lastReplicatorIter) ? Amount(remainder) :
					Amount(driveBalance * (Billing_End - replicatorIter->second.Start).unwrap() / sumTime);
				remainder -= replicatorReward.unwrap();
				expectedAccounts.at(entry.key()).Balances.debit(Storage_Mosaic_Id, replicatorReward, Billing_End);
				expectedAccounts.at(replicatorKey).Balances.credit(Storage_Mosaic_Id, replicatorReward, Billing_End);
				entry.billingHistory().back().Payments.emplace_back(state::PaymentInformation{ replicatorKey, Amount(replicatorReward), Billing_End });
			}

			return entry;
		}

		struct CacheValues {
		public:
			CacheValues() : InitialDriveEntry(Key()), ExpectedDriveEntry(Key())
			{}

		public:
			state::DriveEntry InitialDriveEntry;
			state::DriveEntry ExpectedDriveEntry;
			std::map<Key, state::AccountState> InitialAccounts;
			std::map<Key, state::AccountState> ExpectedAccounts;
		};

		void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight) {
			// Arrange:
			ObserverTestContext context(mode, currentHeight);
			auto notification = test::CreateBlockNotification();
			auto pObserver = CreateEndBillingObserver(Storage_Mosaic_Id);
			auto& driveCache = context.cache().sub<cache::DriveCache>();
			auto& accountCache = context.cache().sub<cache::AccountStateCache>();

			// Populate cache.
			driveCache.insert(values.InitialDriveEntry);
			driveCache.addBillingDrive(values.InitialDriveEntry.key(), Billing_End);
			for (const auto& initialAccount : values.InitialAccounts) {
				accountCache.addAccount(initialAccount.second);
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
		}
	}

	TEST(TEST_CLASS, EndBilling_Commit_WrongHeight) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(values.InitialAccounts, state::DriveState::InProgress);
		values.ExpectedAccounts = values.InitialAccounts;
		values.ExpectedDriveEntry = values.InitialDriveEntry;

		// Assert:
		RunTest(NotifyMode::Commit, values, Billing_End + Height(1));
	}

	TEST(TEST_CLASS, EndBilling_Commit_WrongState) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(values.InitialAccounts, state::DriveState::Finished);
		values.ExpectedAccounts = values.InitialAccounts;
		values.ExpectedDriveEntry = values.InitialDriveEntry;

		// Assert:
		RunTest(NotifyMode::Commit, values, Billing_End);
	}

	TEST(TEST_CLASS, EndBilling_Commit) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(values.InitialAccounts, state::DriveState::InProgress);
		values.ExpectedAccounts = values.InitialAccounts;
		values.ExpectedDriveEntry = CreateExpectedDriveEntry(values.InitialDriveEntry, values.ExpectedAccounts);

		// Assert:
		RunTest(NotifyMode::Commit, values, Billing_End);
	}

	TEST(TEST_CLASS, EndBilling_Rollback_WrongHeight) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.ExpectedAccounts, state::DriveState::InProgress);
		values.InitialAccounts = values.ExpectedAccounts;
		values.InitialDriveEntry = values.ExpectedDriveEntry;

		// Assert:
		RunTest(NotifyMode::Rollback, values, Billing_End + Height(1));
	}

	TEST(TEST_CLASS, EndBilling_Rollback_WrongState) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.ExpectedAccounts, state::DriveState::NotStarted);
		values.InitialAccounts = values.ExpectedAccounts;
		values.InitialDriveEntry = values.ExpectedDriveEntry;

		// Assert:
		RunTest(NotifyMode::Rollback, values, Billing_End);
	}

	TEST(TEST_CLASS, EndBilling_Rollback) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.ExpectedAccounts, state::DriveState::InProgress);
		values.InitialAccounts = values.ExpectedAccounts;
		values.InitialDriveEntry = CreateExpectedDriveEntry(values.ExpectedDriveEntry, values.InitialAccounts);

		// Assert:
		RunTest(NotifyMode::Rollback, values, Billing_End);
	}
}}
