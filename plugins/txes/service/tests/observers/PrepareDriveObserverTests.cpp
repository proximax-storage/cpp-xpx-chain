/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS PrepareDriveObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(PrepareDriveV1, )
	DEFINE_COMMON_OBSERVER_TESTS(PrepareDriveV2, )

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

		template<VersionType Version>
		std::unique_ptr<model::PrepareDriveNotification<Version>> CreateNotification() {
			return std::make_unique<model::PrepareDriveNotification<Version>>(model::PrepareDriveNotification<Version>(
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

	struct PrepareObserverV1Traits {
		static constexpr VersionType Version = 1;
		static auto CreateObserver() {
			return CreatePrepareDriveV1Observer();
		}

		static std::unique_ptr<state::DriveEntry> CreateDriveEntry() {
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
	};

	struct PrepareObserverV2Traits {
		static constexpr VersionType Version = 2;
		static auto CreateObserver() {
			return CreatePrepareDriveV2Observer();
		}

		static std::unique_ptr<state::DriveEntry> CreateDriveEntry() {
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
			pEntry->setVersion(3);

			return pEntry;
		}
	};

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<PrepareObserverV1Traits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<PrepareObserverV2Traits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TRAITS_BASED_TEST(PrepareDrive_Commit) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Commit, Current_Height);
		auto pNotification = CreateNotification<TTraits::Version>();
		auto pObserver = TTraits::CreateObserver();
		auto& driveCache = context.cache().sub<cache::DriveCache>();
		auto pExpectedDriveEntry = TTraits::CreateDriveEntry();

		// Act:
		test::ObserveNotification(*pObserver, *pNotification, context);

		// Assert: check the cache
		auto iter = driveCache.find(pExpectedDriveEntry->key());
		const auto& actualEntry = iter.get();
		test::AssertEqualDriveData(*pExpectedDriveEntry, actualEntry);
	}

	TRAITS_BASED_TEST(PrepareDrive_Rollback) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Rollback, Current_Height);
		auto pNotification = CreateNotification<TTraits::Version>();
		auto pObserver = TTraits::CreateObserver();
		auto& driveCache = context.cache().sub<cache::DriveCache>();
		auto pEntry = TTraits::CreateDriveEntry();

		// Populate drive cache.
		driveCache.insert(*pEntry);

		// Act:
		test::ObserveNotification(*pObserver, *pNotification, context);

		// Assert: check the cache
		auto iter = driveCache.find(pEntry->key());
		EXPECT_EQ(nullptr, iter.tryGet());
	}
}}
