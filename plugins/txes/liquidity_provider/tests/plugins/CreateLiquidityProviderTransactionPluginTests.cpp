/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <tests/test/LiquidityProviderTestUtils.h>
#include <catapult/utils/HexParser.h>
#include "src/plugins/CreateLiquidityProviderTransactionPlugin.h"
#include "src/model/CreateLiquidityProviderTransaction.h"
#include "src/model/InternalLiquidityProviderNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "catapult/model/EntityHasher.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "catapult/model/Address.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS CreateLiquidityProviderTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(CreateLiquidityProvider, config::ImmutableConfiguration, 1, 1,)

		const auto Generation_Hash = utils::ParseByteArray<GenerationHash>("CE076EF4ABFBC65B046987429E274EC31506D173E91BF102F16BEB7FB8176230");
		constexpr auto Network_Identifier = NetworkIdentifier::Mijin_Test;

		auto CreateConfiguration() {
			auto config = config::ImmutableConfiguration::Uninitialized();
			config.GenerationHash = Generation_Hash;
			config.NetworkIdentifier = Network_Identifier;
			return config;
		}

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateCreateLiquidityProviderTransaction<typename TTraits::TransactionType>();
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

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_CreateLiquidityProvider,  CreateConfiguration())

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
        ASSERT_EQ(5u, sub.numNotifications());
        auto i = 0u;
        EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]);
        EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]);
        EXPECT_EQ(Core_Balance_Transfer_v1_Notification, sub.notificationTypes()[i++]);
        EXPECT_EQ(Core_Balance_Credit_v1_Notification, sub.notificationTypes()[i++]);
        EXPECT_EQ(LiquidityProvider_Create_Liquidity_Provider_v1_Notification, sub.notificationTypes()[i++]);
	}

	PLUGIN_TEST(CanPublishAccountPublicKeyNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<AccountPublicKeyNotification<1>> sub;
		auto config = CreateConfiguration();
		auto pPlugin = TTraits::CreatePlugin(config);
		auto pTransaction = CreateTransaction<TTraits>();
		auto providerKey = Key(CalculateTransactionHash(*pTransaction, config.GenerationHash, sub).array());

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(2u, sub.numMatchingNotifications());
		{
			const auto& notification = sub.matchingNotifications()[0];
			EXPECT_EQ(providerKey, notification.PublicKey);
		}
		{
			const auto& notification = sub.matchingNotifications()[1];
			EXPECT_EQ(pTransaction->SlashingAccount, notification.PublicKey);
		}
	}

	PLUGIN_TEST(CanPublishBalanceTransferNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<BalanceTransferNotification<1>> sub;
		auto config = CreateConfiguration();
		auto pPlugin = TTraits::CreatePlugin(config);
		auto pTransaction = CreateTransaction<TTraits>();
		auto providerKey = Key(CalculateTransactionHash(*pTransaction, config.GenerationHash, sub).array());
		const auto providerAddress = extensions::CopyToUnresolvedAddress(
				PublicKeyToAddress(providerKey, config.NetworkIdentifier));

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];

		EXPECT_EQ(pTransaction->Signer, notification.Sender);
		EXPECT_EQ(providerAddress, notification.Recipient);
		EXPECT_EQ(config::GetUnresolvedCurrencyMosaicId(config), notification.MosaicId);
		EXPECT_EQ(pTransaction->CurrencyDeposit, notification.Amount);
	}

	PLUGIN_TEST(CanPublishBalanceCreditNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<BalanceCreditNotification<1>> sub;
		auto config = CreateConfiguration();
		auto pPlugin = TTraits::CreatePlugin(config);
		auto pTransaction = CreateTransaction<TTraits>();
		auto providerKey = Key(CalculateTransactionHash(*pTransaction, config.GenerationHash, sub).array());
		const auto providerAddress = extensions::CopyToUnresolvedAddress(
				PublicKeyToAddress(providerKey, config.NetworkIdentifier));

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];

		EXPECT_EQ(providerKey, notification.Sender);
		EXPECT_EQ(pTransaction->ProviderMosaicId, notification.MosaicId);
		EXPECT_EQ(pTransaction->InitialMosaicsMinting, notification.Amount);
	}

	PLUGIN_TEST(CanPublishCreateLiquidityProviderNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<CreateLiquidityProviderNotification<1>> sub;
		auto config = CreateConfiguration();
		auto pPlugin = TTraits::CreatePlugin(config);
		auto pTransaction = CreateTransaction<TTraits>();
		auto providerKey = Key(CalculateTransactionHash(*pTransaction, config.GenerationHash, sub).array());
		const auto providerAddress = extensions::CopyToUnresolvedAddress(
				PublicKeyToAddress(providerKey, config.NetworkIdentifier));

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];

		EXPECT_EQ(providerKey, notification.ProviderKey);
		EXPECT_EQ(pTransaction->Signer, notification.Owner);
		EXPECT_EQ(pTransaction->ProviderMosaicId, notification.ProviderMosaicId);
		EXPECT_EQ(pTransaction->CurrencyDeposit, notification.CurrencyDeposit);
		EXPECT_EQ(pTransaction->InitialMosaicsMinting, notification.InitialMosaicsMinting);
		EXPECT_EQ(pTransaction->SlashingPeriod, notification.SlashingPeriod);
		EXPECT_EQ(pTransaction->WindowSize, notification.WindowSize);
		EXPECT_EQ(pTransaction->SlashingAccount, notification.SlashingAccount);
		EXPECT_EQ(pTransaction->Alpha, notification.Alpha);
		EXPECT_EQ(pTransaction->Beta, notification.Beta);
	}
}}
