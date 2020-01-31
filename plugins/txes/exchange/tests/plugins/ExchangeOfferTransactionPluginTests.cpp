/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/EntityHasher.h"
#include "catapult/utils/MemoryUtils.h"
#include "src/plugins/ExchangeOfferTransactionPlugin.h"
#include "src/model/ExchangeOfferTransaction.h"
#include "src/model/ExchangeNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/ExchangeTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"
#include <limits>

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS ExchangeOfferTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(ExchangeOffer, config::ImmutableConfiguration, 1, 1,)

		constexpr auto Offer_Count = 4u;

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateExchangeTransaction<typename TTraits::TransactionType, model::OfferWithDuration>(
				{
					model::OfferWithDuration{model::Offer{{UnresolvedMosaicId(1), Amount(10)}, Amount(100), model::OfferType::Sell}, BlockDuration(1000)},
					model::OfferWithDuration{model::Offer{{UnresolvedMosaicId(2), Amount(20)}, Amount(200), model::OfferType::Buy}, BlockDuration(2000)},
					model::OfferWithDuration{model::Offer{{UnresolvedMosaicId(3), Amount(30)}, Amount(300), model::OfferType::Sell}, BlockDuration(3000)},
					model::OfferWithDuration{model::Offer{{UnresolvedMosaicId(4), Amount(40)}, Amount(400), model::OfferType::Buy}, BlockDuration(4000)},
				}
			);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_Exchange_Offer, test::MutableBlockchainConfiguration().ToConst().Immutable)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		test::MutableBlockchainConfiguration config;
		auto pPlugin = TTraits::CreatePlugin(config.Immutable);
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		auto realSize = pPlugin->calculateRealSize(*pTransaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + Offer_Count * sizeof(model::OfferWithDuration), realSize);
	}

	// region publish - basic

	PLUGIN_TEST(PublishesNoNotificationWhenTransactionVersionIsInvalid) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		test::MutableBlockchainConfiguration config;
		auto pPlugin = TTraits::CreatePlugin(config.Immutable);

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
		test::MutableBlockchainConfiguration config;
		auto pPlugin = TTraits::CreatePlugin(config.Immutable);

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(4, sub.numNotifications());
		EXPECT_EQ(Exchange_Offer_v1_Notification, sub.notificationTypes()[0]);
		for (int i = 1; i < 4; ++i)
			EXPECT_EQ(Core_Balance_Debit_v1_Notification, sub.notificationTypes()[i]);
	}

	// endregion

	// region publish - offer notification

	PLUGIN_TEST(CanPublishOfferNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<OfferNotification<1>> sub;
		test::MutableBlockchainConfiguration config;
		auto pPlugin = TTraits::CreatePlugin(config.Immutable);
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.Owner);
		EXPECT_EQ(Offer_Count, notification.OfferCount);
		EXPECT_EQ_MEMORY(pTransaction->OffersPtr(), notification.OffersPtr, Offer_Count * sizeof(model::OfferWithDuration));
	}

	// endregion

	// region publish - balance debit notifications

	PLUGIN_TEST(CanPublishBalanceDebitNotifications) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<BalanceDebitNotification<1>> sub;
		test::MutableBlockchainConfiguration config;
		auto pPlugin = TTraits::CreatePlugin(config.Immutable);
		auto pTransaction = CreateTransaction<TTraits>();
		auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(config.Immutable);

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(3u, sub.numMatchingNotifications());

		const auto& notification1 = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification1.Sender);
		EXPECT_EQ(UnresolvedMosaicId(1), notification1.MosaicId);
		EXPECT_EQ(Amount(10), notification1.Amount);

		const auto& notification2 = sub.matchingNotifications()[1];
		EXPECT_EQ(pTransaction->Signer, notification2.Sender);
		EXPECT_EQ(UnresolvedMosaicId(3), notification2.MosaicId);
		EXPECT_EQ(Amount(30), notification2.Amount);

		const auto& notification3 = sub.matchingNotifications()[2];
		EXPECT_EQ(pTransaction->Signer, notification3.Sender);
		EXPECT_EQ(currencyMosaicId, notification3.MosaicId);
		EXPECT_EQ(Amount(600), notification3.Amount);
	}

	// endregion
}}
