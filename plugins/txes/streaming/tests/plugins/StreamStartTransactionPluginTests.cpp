/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "sdk/src/extensions/ConversionExtensions.h"
#include <catapult/model/Address.h>
#include "catapult/utils/HexParser.h"
#include "src/plugins/StreamStartTransactionPlugin.h"
#include "src/model/StreamStartTransaction.h"
#include "src/catapult/model/StorageNotifications.h"
#include "catapult/model/StreamingNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/StreamingTestUtils.h"
#include "tests/test/nodeps/TestConstants.h"
#include <src/catapult/model/LiquidityProviderNotifications.h>

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS StreamStartTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(StreamStart, config::ImmutableConfiguration, 1, 1,)

		const auto Generation_Hash = utils::ParseByteArray<GenerationHash>("CE076EF4ABFBC65B046987429E274EC31506D173E91BF102F16BEB7FB8176230");
		constexpr auto Network_Identifier = NetworkIdentifier::Mijin_Test;
		constexpr auto Folder_Name_Size = 10u;

		auto CreateConfiguration() {
			auto config = config::ImmutableConfiguration::Uninitialized();
			config.GenerationHash = Generation_Hash;
			config.NetworkIdentifier = Network_Identifier;
			config.CurrencyMosaicId = test::Default_Currency_Mosaic_Id;
			config.StreamingMosaicId = test::Default_Streaming_Mosaic_Id;
			return config;
		}

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateStreamStartTransaction<typename TTraits::TransactionType>();
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , Entity_Type_StreamStart, CreateConfiguration())

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();
		pTransaction->FolderNameSize = Folder_Name_Size;

		// Act:
		auto realSize = pPlugin->calculateRealSize(*pTransaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + Folder_Name_Size, realSize);
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
		auto i = 0u;
		EXPECT_EQ(Storage_Drive_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Storage_Stream_Start_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Streaming_Stream_Start_Folder_Name_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Balance_Transfer_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(LiquidityProvider_Credit_Mosaic_v1_Notification, sub.notificationTypes()[i++]);
	}

	// endregion

	// region publish - drive notification

	PLUGIN_TEST(CanPublishSDriveNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<DriveNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
		EXPECT_EQ(Entity_Type_StreamStart, notification.TransactionType);
	}

	// endregion

	// region publish - stream start notification

	PLUGIN_TEST(CanPublishStreamStartNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<StreamStartNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
		EXPECT_EQ(pTransaction->ExpectedUploadSizeMegabytes, notification.ExpectedUploadSize);
		EXPECT_EQ(pTransaction->FolderNameSize, notification.FolderName.size());
		EXPECT_EQ_MEMORY(pTransaction->FolderNamePtr(), notification.FolderName.data(), notification.FolderName.size());
	}

	// endregion

	PLUGIN_TEST(CanPublishStreamStartFolderName) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<StreamStartFolderNameNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->FolderNameSize, notification.FolderNameSize);
	}

	// endregion

	// region publish - balance transfer notification

	PLUGIN_TEST(CanPublishBalanceTransferNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<BalanceTransferNotification<1>> sub;
		const auto& config = CreateConfiguration();
		auto pPlugin = TTraits::CreatePlugin(config);
		auto pTransaction = CreateTransaction<TTraits>();
		constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;
		auto driveAddress = extensions::CopyToUnresolvedAddress(
				PublicKeyToAddress(pTransaction->DriveKey, config.NetworkIdentifier));

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];

		EXPECT_EQ(pTransaction->Signer, notification.Sender);
		EXPECT_EQ(driveAddress, notification.Recipient);
		EXPECT_EQ(config.CurrencyMosaicId.unwrap(), notification.MosaicId.unwrap());
		EXPECT_EQ(pTransaction->FeedbackFeeAmount, notification.Amount);
	}

	// endregion

	// region publish - credit mosaic notification

	PLUGIN_TEST(CanPublishCreditMosaicNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<CreditMosaicNotification<1>> sub;
		const auto& config = CreateConfiguration();
		auto pPlugin = TTraits::CreatePlugin(config);
		auto pTransaction = CreateTransaction<TTraits>();
		constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;
		auto driveAddress = extensions::CopyToUnresolvedAddress(
				PublicKeyToAddress(pTransaction->DriveKey, config.NetworkIdentifier));

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];

		EXPECT_EQ(pTransaction->Signer, notification.CurrencyDebtor);
		EXPECT_EQ(config.StreamingMosaicId.unwrap(), notification.MosaicId.unwrap());
		EXPECT_EQ(UnresolvedAmountType::StreamingWork, notification.MosaicAmount.Type);

		auto pActualAmount = (model::StreamingWork *) notification.MosaicAmount.DataPtr;
		EXPECT_EQ(pTransaction->DriveKey, pActualAmount->DriveKey);
		EXPECT_EQ(pTransaction->ExpectedUploadSizeMegabytes, pActualAmount->UploadSize);
	}

	// endregion
}}