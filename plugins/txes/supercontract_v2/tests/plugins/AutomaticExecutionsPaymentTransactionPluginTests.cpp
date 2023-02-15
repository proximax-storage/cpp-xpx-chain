/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/AutomaticExecutionsPaymentTransactionPlugin.h"
#include "src/model/AutomaticExecutionsPaymentTransaction.h"
#include "src/catapult/model/SupercontractNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/SuperContractTestUtils.h"
#include <tests/test/nodeps/TestConstants.h>
#include "catapult/utils/HexParser.h"
#include "catapult/model/LiquidityProviderNotifications.h"
#include "sdk/src/extensions/ConversionExtensions.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS AutomaticExecutionsPaymentTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(AutomaticExecutionsPayment, config::ImmutableConfiguration, 1, 1,)

		const auto Generation_Hash = utils::ParseByteArray<GenerationHash>("CE076EF4ABFBC65B046987429E274EC31506D173E91BF102F16BEB7FB8176230");
		constexpr auto Network_Identifier = NetworkIdentifier::Mijin_Test;

		auto CreateConfiguration() {
			auto config = config::ImmutableConfiguration::Uninitialized();
			config.GenerationHash = Generation_Hash;
			config.NetworkIdentifier = Network_Identifier;
			config.CurrencyMosaicId = test::Default_Currency_Mosaic_Id;
			config.StreamingMosaicId = test::Default_Streaming_Mosaic_Id;
			config.SuperContractMosaicId = test::Default_Super_Contract_Mosaic_Id;
			return config;
		}

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateAutomaticExecutionsPaymentTransaction<typename TTraits::TransactionType>();
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_AutomaticExecutionsPaymentTransaction, CreateConfiguration())

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
		ASSERT_EQ(3u, sub.numNotifications());
		auto i = 0u;
		EXPECT_EQ(SuperContract_v2_Automatic_Executions_Replenishment_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(LiquidityProvider_Credit_Mosaic_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(LiquidityProvider_Credit_Mosaic_v1_Notification, sub.notificationTypes()[i++]);
	}

	// endregion

	// region publish - automatic executions payment notification

	PLUGIN_TEST(CanPublishAutomaticExecutionsPaymentNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<AutomaticExecutionsReplenishmentNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->ContractKey, notification.ContractKey);
		EXPECT_EQ(pTransaction->AutomaticExecutionsNumber, notification.Number);
	}

	// endregion

	// region publish - credit mosaic notification

	PLUGIN_TEST(CanPublishCreditMosaicNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<CreditMosaicNotification<1>> sub;
		const auto& config = CreateConfiguration();
		auto pPlugin = TTraits::CreatePlugin(config);
		auto pTransaction = CreateTransaction<TTraits>();
		Hash256 paymentHash;
		crypto::Sha3_256_Builder sha3;
		sha3.update({pTransaction->ContractKey, config.GenerationHash});
		sha3.final(paymentHash);
		auto contractExecutionPaymentKey = Key(paymentHash.array());

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(2u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];

		EXPECT_EQ(pTransaction->Signer, notification.CurrencyDebtor);
		EXPECT_EQ(contractExecutionPaymentKey, notification.MosaicCreditor);
		EXPECT_EQ(notification.MosaicId.unwrap(), config.SuperContractMosaicId.unwrap());
		EXPECT_EQ(notification.MosaicAmount.unwrap(), 0u);

		const auto& notification2 = sub.matchingNotifications()[1];
		EXPECT_EQ(pTransaction->Signer, notification2.CurrencyDebtor);
		EXPECT_EQ(contractExecutionPaymentKey, notification2.MosaicCreditor);
		EXPECT_EQ(notification2.MosaicId.unwrap(), config.StreamingMosaicId.unwrap());
		EXPECT_EQ(notification2.MosaicAmount.unwrap(), 0u);
	}

	// endregion
}}

