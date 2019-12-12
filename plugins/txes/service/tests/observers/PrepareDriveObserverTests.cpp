/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/core/NotificationTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS PrepareDriveObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(PrepareDrive, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::DriveCacheFactory>;
		using Notification = model::PrepareDriveNotification<1>;

		const Key Drive_Key = test::GenerateRandomByteArray<Key>();
		const Key Owner = test::GenerateRandomByteArray<Key>();
		const Height Current_Height = test::GenerateRandomValue<Height>();
		const BlockDuration Duration = test::GenerateRandomValue<BlockDuration>();
		const BlockDuration Billing_Period = test::GenerateRandomValue<BlockDuration>();
		const Amount Billing_Price = test::GenerateRandomValue<Amount>();
		const uint64_t Size = test::Random();
		const uint16_t Replicas = test::Random16();
		const uint16_t Min_Replicators = test::Random16();
		const uint8_t Percent_Approvers = test::RandomByte();

		std::unique_ptr<state::DriveEntry> CreateDriveEntry() {
			auto pEntry = std::make_unique<state::DriveEntry>(state::DriveEntry(Drive_Key));
			pEntry->setState(state::DriveState::NotStarted);
			pEntry->setOwner(Owner);
			pEntry->setStart(Current_Height);
			pEntry->setDuration(Duration);
			pEntry->setBillingPeriod(Billing_Period);
			pEntry->setBillingPrice(Billing_Price);
			pEntry->setSize(Size);
			pEntry->setReplicas(Replicas);
			pEntry->setMinReplicators(Min_Replicators);
			pEntry->setPercentApprovers(Percent_Approvers);

			return pEntry;
		}

		std::unique_ptr<Notification> CreateNotification() {
			return std::make_unique<Notification>(Notification(
				Drive_Key,
				Owner,
				Duration,
				Billing_Period,
				Billing_Price,
				Size,
				Replicas,
				Min_Replicators,
				Percent_Approvers));
		}
	}

	TEST(TEST_CLASS, PrepareDrive_Commit) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Commit, Current_Height);
		auto pNotification = CreateNotification();
		auto pObserver = CreatePrepareDriveObserver();
		auto& driveCache = context.cache().sub<cache::DriveCache>();
		auto pExpectedDriveEntry = CreateDriveEntry();

		// Act:
		test::ObserveNotification(*pObserver, *pNotification, context);

		// Assert: check the cache
		auto iter = driveCache.find(pExpectedDriveEntry->key());
		const auto& actualEntry = iter.get();
		test::AssertEqualDriveData(*pExpectedDriveEntry, actualEntry);
	}

	TEST(TEST_CLASS, PrepareDrive_Rollback) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Rollback, Current_Height);
		auto pNotification = CreateNotification();
		auto pObserver = CreatePrepareDriveObserver();
		auto& driveCache = context.cache().sub<cache::DriveCache>();
		auto pEntry = CreateDriveEntry();

		// Populate drive cache.
		driveCache.insert(*pEntry);

		// Act:
		test::ObserveNotification(*pObserver, *pNotification, context);

		// Assert: check the cache
		auto iter = driveCache.find(pEntry->key());
		EXPECT_EQ(nullptr, iter.tryGet());
	}
}}
