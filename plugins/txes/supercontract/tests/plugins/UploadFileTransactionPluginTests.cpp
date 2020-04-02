/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "src/plugins/UploadFileTransactionPlugin.h"
#include "src/model/SuperContractNotifications.h"
#include "src/model/UploadFileTransaction.h"
#include "plugins/txes/service/src/model/ServiceNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/TestHarness.h"
#include <limits>

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS UploadFileTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(UploadFile, 1, 1,)

		constexpr auto Num_Add_Actions = 3u;
		constexpr auto Num_Remove_Actions = 4u;

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateUploadFileTransaction<typename TTraits::TransactionType>(Num_Add_Actions, Num_Remove_Actions);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS_ONLY_EMBEDDABLE(TEST_CLASS,,,Entity_Type_UploadFile)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		auto realSize = pPlugin->calculateRealSize(*pTransaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) +
			Num_Add_Actions * sizeof(AddAction) + Num_Remove_Actions * sizeof(RemoveAction), realSize);
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
		ASSERT_EQ(3u, sub.numNotifications());
		EXPECT_EQ(SuperContract_SuperContract_v1_Notification, sub.notificationTypes()[0]);
		EXPECT_EQ(Service_Drive_v1_Notification, sub.notificationTypes()[1]);
		EXPECT_EQ(Service_Drive_File_System_v1_Notification, sub.notificationTypes()[2]);
	}

	// endregion

	// region super contract notification

	PLUGIN_TEST(CanPublishSuperContractNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<SuperContractNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.SuperContract);
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
		EXPECT_EQ(Entity_Type_UploadFile, notification.TransactionType);
	}

	// endregion

	// region publish - drive file system notification

	PLUGIN_TEST(CanPublishDriveFileSystemNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<DriveFileSystemNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.Signer);
		EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
		EXPECT_EQ(pTransaction->RootHash, notification.RootHash);
		EXPECT_EQ(pTransaction->XorRootHash, notification.XorRootHash);
		EXPECT_EQ(pTransaction->AddActionsCount, notification.AddActionsCount);
		EXPECT_EQ_MEMORY(pTransaction->AddActionsPtr(), notification.AddActionsPtr, Num_Add_Actions * sizeof(AddAction));
        EXPECT_EQ(pTransaction->RemoveActionsCount, notification.RemoveActionsCount);
		EXPECT_EQ_MEMORY(pTransaction->RemoveActionsPtr(), notification.RemoveActionsPtr, Num_Remove_Actions * sizeof(RemoveAction));
	}

	// endregion
}}
