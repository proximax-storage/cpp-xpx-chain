/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <sdk/src/extensions/ConversionExtensions.h>
#include <catapult/model/Address.h>
#include <tests/test/nodeps/TestConstants.h>
#include "catapult/utils/HexParser.h"
#include "src/plugins/PrepareBcDriveTransactionPlugin.h"
#include "src/model/PrepareBcDriveTransaction.h"
#include "src/catapult/model/StorageNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/StorageTestUtils.h"
#include "catapult/model/EntityHasher.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS PrepareBcDriveTransactionPluginTests

	// region TransactionPlugin
	
	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(PrepareBcDrive, config::ImmutableConfiguration, 1, 1,)

		const auto Generation_Hash = utils::ParseByteArray<GenerationHash>("CE076EF4ABFBC65B046987429E274EC31506D173E91BF102F16BEB7FB8176230");
		constexpr auto Network_Identifier = NetworkIdentifier::Mijin_Test;

		auto CreateConfiguration() {
			auto config = config::ImmutableConfiguration::Uninitialized();
			 config.GenerationHash = Generation_Hash;
			 config.NetworkIdentifier = Network_Identifier;
			 config.CurrencyMosaicId = test::Default_Currency_Mosaic_Id;
			 config.StreamingMosaicId = test::Default_Streaming_Mosaic_Id;
			 config.StorageMosaicId = test::Default_Storage_Mosaic_Id;
			return config;
		}

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreatePrepareBcDriveTransaction<typename TTraits::TransactionType>();
		}

		Hash256 CalculateTransactionHash(const model::Transaction& transaction,
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
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , Entity_Type_PrepareBcDrive, CreateConfiguration())

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
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
        ASSERT_EQ(8u, sub.numNotifications());
        auto i = 0u;
		EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Storage_Prepare_Drive_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Balance_Transfer_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Balance_Debit_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Balance_Credit_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Balance_Debit_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Balance_Credit_v1_Notification, sub.notificationTypes()[i++]);
	}

	// endregion

	// region publish - prepare bc drive notification

	PLUGIN_TEST(CanPublishPrepareBcDriveNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<PrepareDriveNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->DriveSize, notification.DriveSize);
		EXPECT_EQ(pTransaction->ReplicatorCount, notification.ReplicatorCount);
	}

	// endregion

	// region publish - account public key notification

	PLUGIN_TEST(CanPublishAccountPublicKeyNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<AccountPublicKeyNotification<1>> sub;
		const auto& config = CreateConfiguration();
		auto pPlugin = TTraits::CreatePlugin(config);
		auto pTransaction = CreateTransaction<TTraits>();
		constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;
		auto driveKey = Key(CalculateTransactionHash(*pTransaction, config.GenerationHash, sub).array());

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(2u, sub.numMatchingNotifications());
		auto i = 0u;
		EXPECT_EQ(driveKey, sub.matchingNotifications()[i++].PublicKey);
		EXPECT_EQ(pTransaction->Signer, sub.matchingNotifications()[i++].PublicKey);
	}

	// endregion
}}
