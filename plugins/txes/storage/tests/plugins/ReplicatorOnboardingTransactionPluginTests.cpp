/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/EntityHasher.h"
#include "catapult/utils/HexParser.h"
#include "src/plugins/ReplicatorOnboardingTransactionPlugin.h"
#include "src/model/ReplicatorOnboardingTransaction.h"
#include "src/catapult/model/LiquidityProviderNotifications.h"
#include "src/catapult/model/StorageNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/nodeps/TestConstants.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/StorageTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS ReplicatorOnboardingTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(ReplicatorOnboarding, config::ImmutableConfiguration, 1, 1,)

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
			return test::CreateReplicatorOnboardingTransaction<typename TTraits::TransactionType>();
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

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_ReplicatorOnboarding, CreateConfiguration())

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
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
        ASSERT_EQ(4u, sub.numNotifications());
        auto i = 0u;
		EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(LiquidityProvider_Credit_Mosaic_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(LiquidityProvider_Credit_Mosaic_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Storage_Replicator_Onboarding_v1_Notification, sub.notificationTypes()[i++]);
	}

	// endregion

	// region publish - replicator onboarding notification

	PLUGIN_TEST(CanPublishReplicatorOnboardingNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ReplicatorOnboardingNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
        EXPECT_EQ(pTransaction->Capacity, notification.Capacity);
	}

	// endregion

	// region publish - account public key notification

	PLUGIN_TEST(CanPublishAccountPublicKeyNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<AccountPublicKeyNotification<1>> sub;
		const auto& config = CreateConfiguration();
		auto pPlugin = TTraits::CreatePlugin(config);
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(2u, sub.numMatchingNotifications());
		auto i = 0u;
		EXPECT_EQ(pTransaction->Signer, sub.matchingNotifications()[i++].PublicKey);
		EXPECT_EQ(pTransaction->Signer, sub.matchingNotifications()[i++].PublicKey);
	}

	// endregion

	// region publish - credit mosaic notification

	PLUGIN_TEST(CanPublishCreditMosaicNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<CreditMosaicNotification<1>> sub;
		const auto& config = CreateConfiguration();
		auto pPlugin = TTraits::CreatePlugin(config);
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(2u, sub.numMatchingNotifications());

		const auto& notification_1 = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification_1.CurrencyDebtor);
		EXPECT_EQ(pTransaction->Signer, notification_1.MosaicCreditor);
		EXPECT_EQ(notification_1.MosaicId.unwrap(), config.StorageMosaicId.unwrap());
		EXPECT_EQ(pTransaction->Capacity, notification_1.MosaicAmount);

		const auto& notification_2 = sub.matchingNotifications()[1];
		EXPECT_EQ(pTransaction->Signer, notification_2.CurrencyDebtor);
		EXPECT_EQ(pTransaction->Signer, notification_2.MosaicCreditor);
		EXPECT_EQ(notification_2.MosaicId.unwrap(), config.StreamingMosaicId.unwrap());
		EXPECT_EQ(Amount(2 * pTransaction->Capacity.unwrap()), notification_2.MosaicAmount);
	}

	// endregion
}}
