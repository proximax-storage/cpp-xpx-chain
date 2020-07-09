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

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS ExchangeOfferTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(ExchangeOffer, config::ImmutableConfiguration, 4, 4,)

		constexpr auto Offer_Count = 4u;

		template<typename TTraits>
		auto CreateTransaction(VersionType version) {
			return test::CreateExchangeTransaction<typename TTraits::TransactionType, model::OfferWithDuration>(
				{
					model::OfferWithDuration{model::Offer{{UnresolvedMosaicId(1), Amount(10)}, Amount(100), model::OfferType::Sell}, BlockDuration(1000)},
					model::OfferWithDuration{model::Offer{{UnresolvedMosaicId(2), Amount(20)}, Amount(200), model::OfferType::Buy}, BlockDuration(2000)},
					model::OfferWithDuration{model::Offer{{UnresolvedMosaicId(3), Amount(30)}, Amount(300), model::OfferType::Sell}, BlockDuration(3000)},
					model::OfferWithDuration{model::Offer{{UnresolvedMosaicId(4), Amount(40)}, Amount(400), model::OfferType::Buy}, BlockDuration(4000)},
				},
				version
			);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_Exchange_Offer, test::MutableBlockchainConfiguration().ToConst().Immutable)

	namespace {
		template<typename TTraits>
		void AssertCanCalculateSize(VersionType version) {
			// Arrange:
			test::MutableBlockchainConfiguration config;
			auto pPlugin = TTraits::CreatePlugin(config.Immutable);
			auto pTransaction = CreateTransaction<TTraits>(version);

			// Act:
			auto realSize = pPlugin->calculateRealSize(*pTransaction);

			// Assert:
			EXPECT_EQ(sizeof(typename TTraits::TransactionType) + Offer_Count * sizeof(model::OfferWithDuration), realSize);
		}
	}

	PLUGIN_TEST(CanCalculateSize_v1) {
		AssertCanCalculateSize<TTraits>(1);
	}

	PLUGIN_TEST(CanCalculateSize_v2) {
		AssertCanCalculateSize<TTraits>(2);
	}

	PLUGIN_TEST(CanCalculateSize_v3) {
		AssertCanCalculateSize<TTraits>(3);
	}

	PLUGIN_TEST(CanCalculateSize_v4) {
		AssertCanCalculateSize<TTraits>(4);
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

	namespace {
		template<typename TTraits>
		void AssertCanPublishCorrectNumberOfNotifications(VersionType version, NotificationType expectedExchangeOfferType) {
			// Arrange:
			auto pTransaction = CreateTransaction<TTraits>(version);
			mocks::MockNotificationSubscriber sub;
			test::MutableBlockchainConfiguration config;
			auto pPlugin = TTraits::CreatePlugin(config.Immutable);

			// Act:
			test::PublishTransaction(*pPlugin, *pTransaction, sub);

			// Assert:
			ASSERT_EQ(4, sub.numNotifications());
			EXPECT_EQ(expectedExchangeOfferType, sub.notificationTypes()[0]);
			for (int i = 1; i < 4; ++i)
				EXPECT_EQ(Core_Balance_Debit_v1_Notification, sub.notificationTypes()[i]);
		}
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotifications_v1) {
		AssertCanPublishCorrectNumberOfNotifications<TTraits>(1, Exchange_Offer_v1_Notification);
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotifications_v2) {
		AssertCanPublishCorrectNumberOfNotifications<TTraits>(2, Exchange_Offer_v2_Notification);
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotifications_v3) {
		AssertCanPublishCorrectNumberOfNotifications<TTraits>(3, Exchange_Offer_v3_Notification);
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotifications_v4) {
		AssertCanPublishCorrectNumberOfNotifications<TTraits>(4, Exchange_Offer_v4_Notification);
	}

	// endregion

	// region publish - offer notification

	namespace {
		template<typename TTraits, VersionType version>
		void AssertCanPublishOfferNotification() {
			// Arrange:
			mocks::MockTypedNotificationSubscriber<OfferNotification<version>> sub;
			test::MutableBlockchainConfiguration config;
			auto pPlugin = TTraits::CreatePlugin(config.Immutable);
			auto pTransaction = CreateTransaction<TTraits>(version);

			// Act:
			test::PublishTransaction(*pPlugin, *pTransaction, sub);

			// Assert:
			ASSERT_EQ(1u, sub.numMatchingNotifications());
			const auto& notification = sub.matchingNotifications()[0];
			EXPECT_EQ(pTransaction->Signer, notification.Owner);
			EXPECT_EQ(Offer_Count, notification.OfferCount);
			EXPECT_EQ_MEMORY(pTransaction->OffersPtr(), notification.OffersPtr, Offer_Count * sizeof(model::OfferWithDuration));
		}
	}

	PLUGIN_TEST(CanPublishOfferNotification_v1) {
		AssertCanPublishOfferNotification<TTraits, 1>();
	}

	PLUGIN_TEST(CanPublishOfferNotification_v2) {
		AssertCanPublishOfferNotification<TTraits, 2>();
	}

	PLUGIN_TEST(CanPublishOfferNotification_v3) {
		AssertCanPublishOfferNotification<TTraits, 3>();
	}

	PLUGIN_TEST(CanPublishOfferNotification_v4) {
		AssertCanPublishOfferNotification<TTraits, 4>();
	}

	// endregion

	// region publish - balance debit notifications

	namespace {
		template<typename TTraits>
		void AssertCanPublishBalanceDebitNotifications(VersionType version) {
			// Arrange:
			mocks::MockTypedNotificationSubscriber<BalanceDebitNotification<1>> sub;
			test::MutableBlockchainConfiguration config;
			auto pPlugin = TTraits::CreatePlugin(config.Immutable);
			auto pTransaction = CreateTransaction<TTraits>(version);
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
	}

	PLUGIN_TEST(CanPublishBalanceDebitNotifications_v1) {
		AssertCanPublishBalanceDebitNotifications<TTraits>(1);
	}

	PLUGIN_TEST(CanPublishBalanceDebitNotifications_v2) {
		AssertCanPublishBalanceDebitNotifications<TTraits>(2);
	}

	PLUGIN_TEST(CanPublishBalanceDebitNotifications_v3) {
		AssertCanPublishBalanceDebitNotifications<TTraits>(3);
	}

	PLUGIN_TEST(CanPublishBalanceDebitNotifications_v4) {
		AssertCanPublishBalanceDebitNotifications<TTraits>(4);
	}

	// endregion
}}
