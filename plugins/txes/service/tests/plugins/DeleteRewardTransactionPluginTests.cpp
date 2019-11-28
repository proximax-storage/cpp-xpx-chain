/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/DeleteRewardTransactionPlugin.h"
#include "src/model/DeleteRewardTransaction.h"
#include "src/model/ServiceNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/TestHarness.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS DeleteRewardTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(DeleteReward, 1, 1,)

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateDeleteRewardTransaction<typename TTraits::TransactionType>(5, 10);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_DeleteReward)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		auto realSize = pPlugin->calculateRealSize(*pTransaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + 5 * (sizeof(DeletedFile)  + 10 * sizeof(ReplicatorUploadInfo)), realSize);
	}

	// region publish - basic

	PLUGIN_TEST(PublishesNoNotificationWhenTransactionVersionIsInvalid) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin();

		typename TTraits::TransactionType transaction;
		transaction.Version = MakeVersion(NetworkIdentifier::Mijin_Test, std::numeric_limits<uint32_t>::max());

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
		ASSERT_EQ(7, sub.numNotifications());
		EXPECT_EQ(Service_Drive_v1_Notification, sub.notificationTypes()[0]);
		EXPECT_EQ(Service_DeleteReward_v1_Notification, sub.notificationTypes()[1]);
		for (auto i = 2u; i < 7; ++i)
			EXPECT_EQ(Service_Reward_v1_Notification, sub.notificationTypes()[i]);
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
		EXPECT_EQ(Entity_Type_DeleteReward, notification.TransactionType);
	}

	// endregion

	// region publish - delete reward notification

	PLUGIN_TEST(CanPublishDeleteRewardNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<DeleteRewardNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		auto pDeletedFile = reinterpret_cast<uint8_t*>(pTransaction->TransactionsPtr());
		size_t deletedFileSize = sizeof(DeletedFile)  + 10 * sizeof(ReplicatorUploadInfo);
		for (auto i = 0u; i < 5u; ++i, pDeletedFile += deletedFileSize)
			EXPECT_EQ_MEMORY(notification.DeletedFiles[i], pDeletedFile, deletedFileSize);
	}

	// endregion

	// region publish - reward notification

	PLUGIN_TEST(CanPublishRewardNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<RewardNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(5u, sub.numMatchingNotifications());
		auto pDeletedFile = reinterpret_cast<uint8_t*>(pTransaction->TransactionsPtr());
		size_t deletedFileSize = sizeof(DeletedFile)  + 10 * sizeof(ReplicatorUploadInfo);
		for (auto i = 0u; i < 5u; ++i, pDeletedFile += deletedFileSize) {
			const auto &notification = sub.matchingNotifications()[i];
			EXPECT_EQ(pTransaction->Signer, notification.DriveKey);
			EXPECT_EQ(reinterpret_cast<const uint8_t*>(notification.DeletedFile), pDeletedFile);
			EXPECT_EQ_MEMORY(notification.DeletedFile, pDeletedFile, deletedFileSize);
		}
	}

	// endregion
}}
