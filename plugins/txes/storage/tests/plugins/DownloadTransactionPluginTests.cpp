/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <sdk/src/extensions/ConversionExtensions.h>
#include <catapult/model/Address.h>
#include <tests/test/nodeps/TestConstants.h>
#include "catapult/utils/HexParser.h"
#include "src/plugins/DownloadTransactionPlugin.h"
#include "src/model/DownloadTransaction.h"
#include "src/catapult/model/LiquidityProviderNotifications.h"
#include "src/catapult/model/StorageNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/StorageTestUtils.h"
#include "catapult/model/EntityHasher.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS DownloadTransactionPluginTests

	// region TransactionPlugin
	
	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(Download, config::ImmutableConfiguration, 1, 1,)

		const auto Generation_Hash = utils::ParseByteArray<GenerationHash>("CE076EF4ABFBC65B046987429E274EC31506D173E91BF102F16BEB7FB8176230");
		constexpr auto Network_Identifier = NetworkIdentifier::Mijin_Test;

		auto CreateConfiguration() {
			auto config = config::ImmutableConfiguration::Uninitialized();
			config.GenerationHash = Generation_Hash;
			config.NetworkIdentifier = Network_Identifier;
			config.CurrencyMosaicId = test::Default_Currency_Mosaic_Id;
			config.StreamingMosaicId = test::Default_Streaming_Mosaic_Id;
			return config;
		}

		Hash256 CalculateTransactionHash(const Transaction& transaction,
										 const GenerationHash& generationHash,
										 model::NotificationSubscriber& sub) {
			return CalculateHash(transaction, generationHash);
		}

		Hash256 CalculateTransactionHash(const model::EmbeddedTransaction& transaction,
										 const GenerationHash& generationHash,
										 model::NotificationSubscriber& sub) {
			Timestamp deadline;
			return CalculateHash(ConvertEmbeddedTransaction(transaction, deadline, sub.mempool()), generationHash);
		}

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateDownloadTransaction<typename TTraits::TransactionType>();
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , Entity_Type_Download, CreateConfiguration())

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		auto realSize = pPlugin->calculateRealSize(*pTransaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + pTransaction->ListOfPublicKeysSize * Key_Size, realSize);
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
        ASSERT_EQ(4u, sub.numNotifications());
        auto i = 0u;
		EXPECT_EQ(Storage_Download_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Balance_Transfer_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(LiquidityProvider_Credit_Mosaic_v1_Notification, sub.notificationTypes()[i++]);
	}

	// endregion

	// region publish - download notification

	PLUGIN_TEST(CanPublishDownloadNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<DownloadNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
        EXPECT_EQ(pTransaction->DownloadSizeMegabytes, notification.DownloadSizeMegabytes);
	}

	// endregion

	// region publish - balance transfer notification

	void GG(const model::Transaction& transaction) {

	}

	PLUGIN_TEST(CanPublishBalanceTransferNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<BalanceTransferNotification<1>> sub;
		const auto& config = CreateConfiguration();
		auto pPlugin = TTraits::CreatePlugin(config);
		auto pTransaction = CreateTransaction<TTraits>();
		auto downloadChannelId = CalculateTransactionHash(*pTransaction, config.GenerationHash, sub);
		const auto downloadChannelKey = Key(downloadChannelId.array());
		const auto downloadChannelAddress = extensions::CopyToUnresolvedAddress
				(PublicKeyToAddress(downloadChannelKey, config.NetworkIdentifier));

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);


		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];

		EXPECT_EQ(pTransaction->Signer, notification.Sender);
		EXPECT_EQ(downloadChannelAddress, notification.Recipient);
		EXPECT_EQ(config.CurrencyMosaicId.unwrap(), notification.MosaicId.unwrap());
		EXPECT_EQ(UnresolvedAmountType::Default, notification.Amount.Type);
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
		auto downloadChannelId = CalculateTransactionHash(*pTransaction, config.GenerationHash, sub);
		const auto downloadChannelKey = Key(downloadChannelId.array());

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];

		EXPECT_EQ(pTransaction->Signer, notification.CurrencyDebtor);
		EXPECT_EQ(downloadChannelKey, notification.MosaicCreditor);
		EXPECT_EQ(notification.MosaicId.unwrap(), config.StreamingMosaicId.unwrap());
		EXPECT_EQ(pTransaction->DownloadSizeMegabytes, notification.MosaicAmount.unwrap());
	}

	// endregion
}}
