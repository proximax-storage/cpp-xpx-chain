/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/EndFileDownloadTransactionPlugin.h"
#include "src/model/EndFileDownloadTransaction.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/ServiceTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS EndFileDownloadTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(EndFileDownload, config::ImmutableConfiguration, 1, 1,)

		constexpr UnresolvedMosaicId Review_Mosaic_Id(1234);
		constexpr UnresolvedMosaicId Streaming_Mosaic_Id(5678);
		constexpr auto Network_Identifier = NetworkIdentifier::Mijin_Test;
		constexpr auto Num_Files = 3u;
		constexpr auto File_Size = 10u;

		auto CreateConfiguration() {
			auto config = config::ImmutableConfiguration::Uninitialized();
			config.ReviewMosaicId = MosaicId(Review_Mosaic_Id.unwrap());
			config.StreamingMosaicId = MosaicId(Streaming_Mosaic_Id.unwrap());
			config.NetworkIdentifier = Network_Identifier;
			return config;
		}

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateEndFileDownloadTransaction<typename TTraits::TransactionType>(Num_Files);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(
		TEST_CLASS,
		,
		,Entity_Type_EndFileDownload,
		CreateConfiguration())

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		auto realSize = pPlugin->calculateRealSize(*pTransaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + Num_Files * sizeof(DownloadAction), realSize);
	}

	// region publish - basic

	PLUGIN_TEST(PublishesNoNotificationWhenTransactionVersionIsInvalid) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());

		typename TTraits::TransactionType transaction;
		transaction.Size = sizeof(transaction);
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
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(5u, sub.numNotifications());
		EXPECT_EQ(Service_Drive_v1_Notification, sub.notificationTypes()[0]);
		EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[1]);
		EXPECT_EQ(Service_EndFileDownload_v1_Notification, sub.notificationTypes()[2]);
		EXPECT_EQ(Core_Balance_Credit_v1_Notification, sub.notificationTypes()[3]);
		EXPECT_EQ(Core_Balance_Credit_v1_Notification, sub.notificationTypes()[4]);
	}

	// endregion

	// region publish - drive notification

	PLUGIN_TEST(CanPublishDriveNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<DriveNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.DriveKey);
		EXPECT_EQ(Entity_Type_EndFileDownload, notification.TransactionType);
	}

	// endregion

	// region publish - end file download notification

	PLUGIN_TEST(CanPublishEndFileDownloadNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<EndFileDownloadNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->FileRecipient, notification.FileRecipient);
		EXPECT_EQ(pTransaction->OperationToken, notification.OperationToken);
		EXPECT_EQ(Num_Files, notification.FileCount);
		EXPECT_EQ_MEMORY(pTransaction->FilesPtr(), notification.FilesPtr, Num_Files * sizeof(DownloadAction));
	}

	// endregion

	// region publish - account public key notification

	PLUGIN_TEST(CanPublishAccountPublicKeyNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<AccountPublicKeyNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->FileRecipient, notification.PublicKey);
	}

	// endregion

	// region publish - balance credit notifications

	PLUGIN_TEST(CanPublishBalanceCreditNotifications) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<BalanceCreditNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(2u, sub.numMatchingNotifications());
		const auto& notification1 = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->FileRecipient, notification1.Sender);
		EXPECT_EQ(Review_Mosaic_Id, notification1.MosaicId);
		EXPECT_EQ(Amount(Num_Files), notification1.Amount);
		const auto& notification2 = sub.matchingNotifications()[1];
		EXPECT_EQ(pTransaction->Signer, notification2.Sender);
		EXPECT_EQ(Streaming_Mosaic_Id, notification2.MosaicId);
		EXPECT_EQ(Amount(Num_Files * File_Size), notification2.Amount);
	}

	// endregion
}}
