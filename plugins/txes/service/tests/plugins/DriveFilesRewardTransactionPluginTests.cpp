/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/DriveFilesRewardTransactionPlugin.h"
#include "src/model/DriveFilesRewardTransaction.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/ServiceTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS DriveFilesRewardTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(DriveFilesReward, 1, 1,)

		constexpr auto Upload_Info_Count = 5u;

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateDriveFilesRewardTransaction<typename TTraits::TransactionType>(Upload_Info_Count);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_DriveFilesReward)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		auto realSize = pPlugin->calculateRealSize(*pTransaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + Upload_Info_Count * sizeof(model::UploadInfo), realSize);
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
		ASSERT_EQ(2, sub.numNotifications());
		EXPECT_EQ(Service_Drive_v1_Notification, sub.notificationTypes()[0]);
		EXPECT_EQ(Service_DriveFilesReward_v1_Notification, sub.notificationTypes()[1]);
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
		EXPECT_EQ(pTransaction->Signer, notification.DriveKey);
		EXPECT_EQ(Entity_Type_DriveFilesReward, notification.TransactionType);
	}

	// endregion

	// region publish - drive files reward notification

	PLUGIN_TEST(CanPublishDriveFilesRewardNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<DriveFilesRewardNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		auto pUploadInfo = pTransaction->UploadInfosPtr();
		for (auto i = 0u; i < Upload_Info_Count; ++i) {
			EXPECT_EQ(notification.UploadInfoPtr[i].Participant, pUploadInfo[i].Participant);
			EXPECT_EQ(notification.UploadInfoPtr[i].Uploaded, pUploadInfo[i].Uploaded);
		}
	}

	// endregion
}}
