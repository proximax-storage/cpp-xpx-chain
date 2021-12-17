/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/DataModificationApprovalTransactionPlugin.h"
#include "src/model/DataModificationApprovalTransaction.h"
#include "src/catapult/model/StorageNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/StorageTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS DataModificationApprovalTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(DataModificationApproval, 1, 1,)

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateDataModificationApprovalTransaction<typename TTraits::TransactionType>();
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_DataModificationApproval)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();
		pTransaction->JudgingKeysCount = 0;
		pTransaction->OverlappingKeysCount = 0;
		pTransaction->JudgedKeysCount = 0;
		pTransaction->OpinionElementCount = 0;

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

	PLUGIN_TEST(CanPublishCorrectNumberOfNotifications) {
		// Arrange:
		auto pTransaction = CreateTransaction<TTraits>();
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
        ASSERT_EQ(6u, sub.numNotifications());
        auto i = 0u;
        EXPECT_EQ(Storage_Drive_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Storage_Opinion_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Storage_Data_Modification_Approval_Refund_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Storage_Data_Modification_Approval_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Storage_Data_Modification_Approval_Download_Work_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Storage_Data_Modification_Approval_Upload_Work_v1_Notification, sub.notificationTypes()[i++]);
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
		EXPECT_EQ(Entity_Type_DataModificationApproval, notification.TransactionType);
	}

	// endregion

	// region publish - data modification approval notification

	PLUGIN_TEST(CanPublishDataModificationApprovalNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<DataModificationApprovalNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
        EXPECT_EQ(pTransaction->DataModificationId, notification.DataModificationId);
        EXPECT_EQ(pTransaction->FileStructureCdi, notification.FileStructureCdi);
        EXPECT_EQ(pTransaction->FileStructureSize, notification.FileStructureSize);
        EXPECT_EQ(pTransaction->UsedDriveSize, notification.UsedDriveSize);
	}

	// endregion
}}
