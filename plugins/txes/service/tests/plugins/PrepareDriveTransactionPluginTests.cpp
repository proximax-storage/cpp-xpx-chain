/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/PrepareDriveTransactionPlugin.h"
#include "src/model/PrepareDriveTransaction.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/ServiceTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS PrepareDriveTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(PrepareDrive, 3, 3,)

		template<typename TTraits>
		auto CreateTransaction(VersionType version) {
			return test::CreatePrepareDriveTransaction<typename TTraits::TransactionType>(version);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS_ONLY_EMBEDDABLE(TEST_CLASS,,, Entity_Type_PrepareDrive)

	namespace {
		template<typename TTraits>
		void AssertCanCalculateSize(VersionType version) {
			// Arrange:
			auto pPlugin = TTraits::CreatePlugin();
			auto pTransaction = CreateTransaction<TTraits>(version);

			// Act:
			auto realSize = pPlugin->calculateRealSize(*pTransaction);

			// Assert:
			EXPECT_EQ(sizeof(typename TTraits::TransactionType), realSize);
		}
	}

	PLUGIN_TEST(CanCalculateSize_v1) {
			AssertCanCalculateSize<TTraits>(1);
	}

	PLUGIN_TEST(CanCalculateSize_v2) {
			AssertCanCalculateSize<TTraits>(2);
	}

	PLUGIN_TEST(CanCalculateSize_v3) {
			AssertCanCalculateSize<TTraits>(3);
	}

	// region publish - basic

	PLUGIN_TEST(PublishesNoNotificationWhenTransactionVersionIsInvalid) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin();

		typename TTraits::TransactionType transaction;
		transaction.Size = sizeof(transaction);
		transaction.Version = MakeVersion(NetworkIdentifier::Mijin_Test, std::numeric_limits<uint32_t>::max());

		// Act:
		test::PublishTransaction(*pPlugin, transaction, sub);

		// Assert:
		ASSERT_EQ(0, sub.numNotifications());
	}

	namespace {
		template<typename TTraits>
		void AssertCanPublishCorrectNumberOfNotifications(VersionType version, NotificationType expectedNotification) {
			// Arrange:
			auto pTransaction = CreateTransaction<TTraits>(version);
			mocks::MockNotificationSubscriber sub;
			auto pPlugin = TTraits::CreatePlugin();

			// Act:
			test::PublishTransaction(*pPlugin, *pTransaction, sub);

			// Assert:
			ASSERT_EQ(1, sub.numNotifications());
			EXPECT_EQ(expectedNotification, sub.notificationTypes()[0]);
		}
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotifications_v1) {
		AssertCanPublishCorrectNumberOfNotifications<TTraits>(1, Service_Prepare_Drive_v1_Notification);
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotifications_v2) {
		AssertCanPublishCorrectNumberOfNotifications<TTraits>(2, Service_Prepare_Drive_v1_Notification);
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotifications_v3) {
		AssertCanPublishCorrectNumberOfNotifications<TTraits>(3, Service_Prepare_Drive_v2_Notification);
	}

	// endregion

	// region publish - prepare drive notification

	namespace {
		template<typename TTraits, VersionType Version>
		void AssertCanPublishPrepareDriveNotification(VersionType version) {
			// Arrange:
			mocks::MockTypedNotificationSubscriber<PrepareDriveNotification<Version>> sub;
			auto pPlugin = TTraits::CreatePlugin();

			auto pTransaction = CreateTransaction<TTraits>(version);
			pTransaction->Signer = test::GenerateRandomByteArray<Key>();

			// Act:
			test::PublishTransaction(*pPlugin, *pTransaction, sub);

			// Assert:
			ASSERT_EQ(1u, sub.numMatchingNotifications());
			const auto& notification = sub.matchingNotifications()[0];
			EXPECT_EQ(pTransaction->Signer, (1 == version) ? notification.Owner : notification.DriveKey);
			if (1 == version)
				EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
			else
				EXPECT_EQ(pTransaction->Owner, notification.Owner);
			EXPECT_EQ(pTransaction->Duration, notification.Duration);
			EXPECT_EQ(pTransaction->BillingPeriod, notification.BillingPeriod);
			EXPECT_EQ(pTransaction->BillingPrice, notification.BillingPrice);
			EXPECT_EQ(pTransaction->DriveSize, notification.DriveSize);
			EXPECT_EQ(pTransaction->Replicas, notification.Replicas);
			EXPECT_EQ(pTransaction->MinReplicators, notification.MinReplicators);
			EXPECT_EQ(pTransaction->PercentApprovers, notification.PercentApprovers);
		}
	}

	PLUGIN_TEST(CanPublishPrepareDriveNotification_v1) {
		AssertCanPublishPrepareDriveNotification<TTraits, 1>(1);
	}

	PLUGIN_TEST(CanPublishPrepareDriveNotification_v2) {
		AssertCanPublishPrepareDriveNotification<TTraits, 1>(2);
	}

	PLUGIN_TEST(CanPublishPrepareDriveNotification_v3) {
		AssertCanPublishPrepareDriveNotification<TTraits, 2>(3);
	}

	// endregion
}}
