/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/EndDriveTransactionPlugin.h"
#include "src/model/EndDriveTransaction.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/ServiceTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS EndDriveTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(EndDrive, 1, 1,)

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateEndDriveTransaction<typename TTraits::TransactionType>();
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_EndDrive)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		auto realSize = pPlugin->calculateRealSize(*pTransaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType), realSize);
	}

	// region publish - basic

	PLUGIN_TEST(PublishesNoNotificationWhenTransactionVersionIsInvalid) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin();

		typename TTraits::TransactionType transaction;
		transaction.Version = MakeVersion(NetworkIdentifier::Mijin_Test, std::numeric_limits<uint32_t>::max());
		transaction.Size = sizeof(transaction);

		// Act:
		test::PublishTransaction(*pPlugin, transaction, sub);

		// Assert:
		ASSERT_EQ(0, sub.numNotifications());
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotifications_SameDriveKey) {
		// Arrange:
		auto pTransaction = CreateTransaction<TTraits>();
		pTransaction->Signer = pTransaction->DriveKey;
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(2, sub.numNotifications());
		EXPECT_EQ(Service_Drive_v1_Notification, sub.notificationTypes()[0]);
		EXPECT_EQ(Service_End_Drive_v1_Notification, sub.notificationTypes()[1]);
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotifications_DifferentDriveKey) {
		// Arrange:
		auto pTransaction = CreateTransaction<TTraits>();
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(3, sub.numNotifications());
		EXPECT_EQ(Service_Drive_v1_Notification, sub.notificationTypes()[0]);
		EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[1]);
		EXPECT_EQ(Service_End_Drive_v1_Notification, sub.notificationTypes()[2]);
	}

	// endregion

	// region publish - drive notification

	PLUGIN_TEST(CanPublishDriveNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<DriveNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
		EXPECT_EQ(Entity_Type_EndDrive, notification.TransactionType);
	}

	// endregion

	// region publish - end drive notification

	PLUGIN_TEST(CanPublishEndDriveNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<EndDriveNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.Signer);
		EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
	}

	// endregion
}}
