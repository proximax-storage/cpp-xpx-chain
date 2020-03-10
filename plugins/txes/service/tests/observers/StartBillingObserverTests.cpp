/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache_core/AccountStateCache.h"
#include "src/observers/Observers.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/core/NotificationTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"
#include <cmath>

namespace catapult { namespace observers {

#define TEST_CLASS StartBillingObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(StartBilling, MosaicId())

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::DriveCacheFactory>;
		using Notification = model::BalanceCreditNotification<1>;

		constexpr auto Storage_Mosaic_Id = MosaicId(1234);
		constexpr Height Current_Height(5);
		constexpr BlockDuration Billing_Period(20);
		constexpr Amount Billing_Price(1000);
		constexpr Amount Credit_Amount(10);
		constexpr auto Drive_Size = 100;

		state::DriveEntry CreateInitialDriveEntry(
				state::AccountState& driveAccount,
				state::DriveState driveState,
				const Amount& driveBalance) {
			state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
			entry.setState(driveState);
			entry.setBillingPeriod(Billing_Period);
			entry.setBillingPrice(Billing_Price);
			driveAccount = test::CreateAccount(entry.key());
			driveAccount.Balances.credit(Storage_Mosaic_Id, driveBalance);

			return entry;
		}

		state::DriveEntry CreateExpectedDriveEntry(state::DriveEntry& initialEntry, state::DriveState driveState) {
			state::DriveEntry entry(initialEntry);
			entry.setState(driveState);
			entry.billingHistory().push_back(state::BillingPeriodDescription{ Current_Height, Current_Height + Height(Billing_Period.unwrap()), {} });
			return entry;
		}

		struct CacheValues {
		public:
			CacheValues()
				: InitialDriveEntry(Key())
				, ExpectedDriveEntry(Key())
				, DriveAccount(Address(), Height())
			{}

		public:
			state::DriveEntry InitialDriveEntry;
			state::DriveEntry ExpectedDriveEntry;
			state::AccountState DriveAccount;
		};

		void RunTest(NotifyMode mode, const CacheValues& values, const Key& driveKey, const Height& currentHeight, const MosaicId& mosaicId) {
			// Arrange:
			ObserverTestContext context(mode, currentHeight);
			Notification notification(driveKey, test::UnresolveXor(mosaicId), UnresolvedAmount(Credit_Amount.unwrap()));
			auto pObserver = CreateStartBillingObserver(Storage_Mosaic_Id);
			auto& driveCache = context.cache().sub<cache::DriveCache>();
			auto& accountCache = context.cache().sub<cache::AccountStateCache>();

			// Populate cache.
			driveCache.insert(values.InitialDriveEntry);
			accountCache.addAccount(values.DriveAccount);

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			auto driveIter = driveCache.find(values.ExpectedDriveEntry.key());
			const auto& actualEntry = driveIter.get();
			test::AssertEqualDriveData(values.ExpectedDriveEntry, actualEntry);
		}
	}

	TEST(TEST_CLASS, StartBilling_Commit) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(values.DriveAccount, state::DriveState::Pending, Billing_Price);
		values.ExpectedDriveEntry = CreateExpectedDriveEntry(values.InitialDriveEntry, state::DriveState::InProgress);

		// Assert:
		RunTest(NotifyMode::Commit, values, values.InitialDriveEntry.key(), Current_Height, Storage_Mosaic_Id);
	}

	TEST(TEST_CLASS, StartBilling_Commit_WrongMosaic) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(values.DriveAccount, state::DriveState::Pending, Billing_Price);
		values.ExpectedDriveEntry = values.InitialDriveEntry;

		// Assert:
		RunTest(NotifyMode::Commit, values, values.InitialDriveEntry.key(), Current_Height, MosaicId(4321));
	}

	TEST(TEST_CLASS, StartBilling_Commit_WrongSender) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(values.DriveAccount, state::DriveState::Pending, Billing_Price);
		values.ExpectedDriveEntry = values.InitialDriveEntry;

		// Assert:
		RunTest(NotifyMode::Commit, values, test::GenerateRandomByteArray<Key>(), Current_Height, Storage_Mosaic_Id);
	}

	TEST(TEST_CLASS, StartBilling_Commit_WrongState) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(values.DriveAccount, state::DriveState::InProgress, Billing_Price);
		values.ExpectedDriveEntry = values.InitialDriveEntry;

		// Assert:
		RunTest(NotifyMode::Commit, values, values.InitialDriveEntry.key(), Current_Height, Storage_Mosaic_Id);
	}

	TEST(TEST_CLASS, StartBilling_Commit_InsufficientBalance) {
		// Arrange:
		CacheValues values;
		values.InitialDriveEntry = CreateInitialDriveEntry(values.DriveAccount, state::DriveState::Pending, Billing_Price - Amount(1));
		values.ExpectedDriveEntry = values.InitialDriveEntry;

		// Assert:
		RunTest(NotifyMode::Commit, values, values.InitialDriveEntry.key(), Current_Height, Storage_Mosaic_Id);
	}

	TEST(TEST_CLASS, StartBilling_Rollback) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.DriveAccount, state::DriveState::Pending, Billing_Price + Credit_Amount - Amount(1));
		values.InitialDriveEntry = CreateExpectedDriveEntry(values.ExpectedDriveEntry, state::DriveState::InProgress);

		// Assert:
		RunTest(NotifyMode::Rollback, values, values.InitialDriveEntry.key(), Current_Height, Storage_Mosaic_Id);
	}

	TEST(TEST_CLASS, StartBilling_Rollback_WrongMosaic) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.DriveAccount, state::DriveState::InProgress, Billing_Price + Credit_Amount - Amount(1));
		values.InitialDriveEntry = values.ExpectedDriveEntry;

		// Assert:
		RunTest(NotifyMode::Rollback, values, values.InitialDriveEntry.key(), Current_Height, MosaicId(4321));
	}

	TEST(TEST_CLASS, StartBilling_Rollback_WrongSender) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.DriveAccount, state::DriveState::InProgress, Billing_Price + Credit_Amount - Amount(1));
		values.InitialDriveEntry = values.ExpectedDriveEntry;

		// Assert:
		RunTest(NotifyMode::Rollback, values, test::GenerateRandomByteArray<Key>(), Current_Height, Storage_Mosaic_Id);
	}

	TEST(TEST_CLASS, StartBilling_Rollback_WrongState) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.DriveAccount, state::DriveState::Pending, Billing_Price + Credit_Amount - Amount(1));
		values.InitialDriveEntry = values.ExpectedDriveEntry;

		// Assert:
		RunTest(NotifyMode::Rollback, values, values.InitialDriveEntry.key(), Current_Height, Storage_Mosaic_Id);
	}

	TEST(TEST_CLASS, StartBilling_Rollback_ExceedingBalance) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.DriveAccount, state::DriveState::Pending, Billing_Price + Credit_Amount);
		values.InitialDriveEntry = values.ExpectedDriveEntry;

		// Assert:
		RunTest(NotifyMode::Rollback, values, values.InitialDriveEntry.key(), Current_Height, Storage_Mosaic_Id);
	}

	TEST(TEST_CLASS, StartBilling_Rollback_InvalidBillingPeriodEnd) {
		// Arrange:
		CacheValues values;
		values.ExpectedDriveEntry = CreateInitialDriveEntry(values.DriveAccount, state::DriveState::Pending, Billing_Price + Credit_Amount - Amount(1));
		values.InitialDriveEntry = CreateExpectedDriveEntry(values.ExpectedDriveEntry, state::DriveState::InProgress);
		values.InitialDriveEntry.billingHistory().back().Start = Current_Height - Height(1);

		// Assert:
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values, values.InitialDriveEntry.key(), Current_Height, Storage_Mosaic_Id), catapult_runtime_error);
	}
}}
