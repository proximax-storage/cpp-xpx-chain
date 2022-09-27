/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <tests/test/LiquidityProviderTestUtils.h>
#include <catapult/utils/HexParser.h>
#include "src/plugins/ManualRateChangeTransactionPlugin.h"
#include "src/model/ManualRateChangeTransaction.h"
#include "src/model/InternalLiquidityProviderNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "catapult/model/EntityHasher.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "catapult/model/Address.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS ManualRateChangeTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(ManualRateChange, 1, 1,)

		const auto Generation_Hash = utils::ParseByteArray<GenerationHash>("CE076EF4ABFBC65B046987429E274EC31506D173E91BF102F16BEB7FB8176230");
		constexpr auto Network_Identifier = NetworkIdentifier::Mijin_Test;

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateManualRateChangeTransaction<typename TTraits::TransactionType>();
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

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_ManualRateChange)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();
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
        ASSERT_EQ(1u, sub.numNotifications());
        auto i = 0u;
        EXPECT_EQ(LiquidityProvider_Manual_Rate_Change_v1_Notification, sub.notificationTypes()[i++]);
	}

	PLUGIN_TEST(CanPublishManualRateChangeProviderNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ManualRateChangeNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];

		EXPECT_EQ(pTransaction->Signer, notification.Signer);
		EXPECT_EQ(pTransaction->ProviderMosaicId, notification.ProviderMosaicId);
		EXPECT_EQ(pTransaction->CurrencyBalanceIncrease, notification.CurrencyBalanceIncrease);
		EXPECT_EQ(pTransaction->CurrencyBalanceChange, notification.CurrencyBalanceChange);
		EXPECT_EQ(pTransaction->MosaicBalanceIncrease, notification.MosaicBalanceIncrease);
		EXPECT_EQ(pTransaction->MosaicBalanceChange, notification.MosaicBalanceChange);
	}
}}
