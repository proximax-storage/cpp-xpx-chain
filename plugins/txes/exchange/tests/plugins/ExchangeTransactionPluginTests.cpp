/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/utils/MemoryUtils.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "src/plugins/ExchangeTransactionPlugin.h"
#include "src/model/ExchangeTransaction.h"
#include "src/model/ExchangeNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/ExchangeTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"
#include <limits>

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS ExchangeTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(Exchange, config::ImmutableConfiguration, 2, 2,)

		constexpr auto Offer_Count = 2u;

		template<typename TTraits, VersionType Version>
		auto CreateTransaction(const Key& offerOwner1 = test::GenerateRandomByteArray<Key>(), const Key& offerOwner2 = test::GenerateRandomByteArray<Key>()) {
			return test::CreateExchangeTransaction<typename TTraits::TransactionType, model::MatchedOffer>(
				{
					model::MatchedOffer{model::Offer{{UnresolvedMosaicId(1), Amount(10)}, Amount(100), model::OfferType::Sell}, offerOwner1},
					model::MatchedOffer{model::Offer{{UnresolvedMosaicId(2), Amount(20)}, Amount(200), model::OfferType::Buy}, offerOwner2},
				},
				Version
			);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_Exchange, test::MutableBlockchainConfiguration().ToConst().Immutable)

	namespace {
		template<typename TTraits, VersionType Version>
		void AssertCanCalculateSize() {
			// Arrange:
			test::MutableBlockchainConfiguration config;
			auto pPlugin = TTraits::CreatePlugin(config.Immutable);
			auto pTransaction = CreateTransaction<TTraits, Version>();

			// Act:
			auto realSize = pPlugin->calculateRealSize(*pTransaction);

			// Assert:
			EXPECT_EQ(sizeof(typename TTraits::TransactionType) + Offer_Count * sizeof(model::MatchedOffer), realSize);
		}
	}

	PLUGIN_TEST(CanCalculateSize_v1) {
		AssertCanCalculateSize<TTraits, 1>();
	}

	PLUGIN_TEST(CanCalculateSize_v2) {
		AssertCanCalculateSize<TTraits, 2>();
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
		template<typename TTraits, VersionType Version>
		void AssertCanPublishCorrectNumberOfNotifications(NotificationType expectedExchangeOfferType) {
			// Arrange:
			auto pTransaction = CreateTransaction<TTraits, Version>();
			mocks::MockNotificationSubscriber sub;
			test::MutableBlockchainConfiguration config;
			auto pPlugin = TTraits::CreatePlugin(config.Immutable);

			// Act:
			test::PublishTransaction(*pPlugin, *pTransaction, sub);

			// Assert:
			ASSERT_EQ(1 + Offer_Count * 2, sub.numNotifications());
			EXPECT_EQ(expectedExchangeOfferType, sub.notificationTypes()[0]);
			for (auto i = 0u; i < Offer_Count; ++i) {
				EXPECT_EQ(Core_Balance_Transfer_v1_Notification, sub.notificationTypes()[2 * i + 1]);
				EXPECT_EQ(Core_Balance_Credit_v1_Notification, sub.notificationTypes()[2 * i + 2]);
			}
		}
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotifications_v1) {
		AssertCanPublishCorrectNumberOfNotifications<TTraits, 1>(Exchange_Exchange_v1_Notification);
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotifications_v2) {
		AssertCanPublishCorrectNumberOfNotifications<TTraits, 2>(Exchange_Exchange_v2_Notification);
	}

	// endregion

	// region publish - exchange notification

	namespace {
		template<typename TTraits, VersionType Version>
		void AssertCanPublishExchangeNotification() {
			// Arrange:
			mocks::MockTypedNotificationSubscriber<ExchangeNotification<Version>> sub;
			test::MutableBlockchainConfiguration config;
			auto pPlugin = TTraits::CreatePlugin(config.Immutable);
			auto pTransaction = CreateTransaction<TTraits, Version>();

			// Act:
			test::PublishTransaction(*pPlugin, *pTransaction, sub);

			// Assert:
			ASSERT_EQ(1u, sub.numMatchingNotifications());
			const auto& notification = sub.matchingNotifications()[0];
			EXPECT_EQ(pTransaction->Signer, notification.Signer);
			EXPECT_EQ(Offer_Count, notification.OfferCount);
			EXPECT_EQ_MEMORY(
					pTransaction->OffersPtr(), notification.OffersPtr, Offer_Count * sizeof(model::MatchedOffer));
		}
	}

	PLUGIN_TEST(CanPublishExchangeNotification_v1) {
		AssertCanPublishExchangeNotification<TTraits, 1>();
	}

	PLUGIN_TEST(CanPublishExchangeNotification_v2) {
		AssertCanPublishExchangeNotification<TTraits, 2>();
	}

	// endregion

	// region publish - balance debit notification

	namespace {
		template<typename TTraits, VersionType Version>
		void AssertCanPublishBalanceTransferNotifications() {
			// Arrange:
			mocks::MockTypedNotificationSubscriber<BalanceTransferNotification<1>> sub;
			test::MutableBlockchainConfiguration config;
			NetworkIdentifier networkIdentifier = NetworkIdentifier::Mijin_Test;
			config.Immutable.NetworkIdentifier = networkIdentifier;
			auto pPlugin = TTraits::CreatePlugin(config.Immutable);

			auto offerOwner1 = test::GenerateRandomByteArray<Key>();
			auto offerOwner1Address =
					extensions::CopyToUnresolvedAddress(PublicKeyToAddress(offerOwner1, networkIdentifier));
			auto offerOwner2 = test::GenerateRandomByteArray<Key>();
			auto offerOwner2Address =
					extensions::CopyToUnresolvedAddress(PublicKeyToAddress(offerOwner2, networkIdentifier));
			auto pTransaction = CreateTransaction<TTraits, Version>(offerOwner1, offerOwner2);
			auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(config.Immutable);

			// Act:
			test::PublishTransaction(*pPlugin, *pTransaction, sub);

			// Assert:
			ASSERT_EQ(2u, sub.numMatchingNotifications());

			const auto& notification1 = sub.matchingNotifications()[0];
			EXPECT_EQ(pTransaction->Signer, notification1.Sender);
			EXPECT_EQ(offerOwner1Address, notification1.Recipient);
			EXPECT_EQ(currencyMosaicId, notification1.MosaicId);
			EXPECT_EQ(Amount(100), notification1.Amount);

			const auto& notification2 = sub.matchingNotifications()[1];
			EXPECT_EQ(pTransaction->Signer, notification2.Sender);
			EXPECT_EQ(offerOwner2Address, notification2.Recipient);
			EXPECT_EQ(UnresolvedMosaicId(2), notification2.MosaicId);
			EXPECT_EQ(Amount(20), notification2.Amount);
		}
	}

	PLUGIN_TEST(CanPublishBalanceTransferNotifications_v1) {
		AssertCanPublishBalanceTransferNotifications<TTraits, 1>();
	}

	PLUGIN_TEST(CanPublishBalanceTransferNotifications_v2) {
		AssertCanPublishBalanceTransferNotifications<TTraits, 2>();
	}

	// endregion

	// region publish - balance credit notification

	namespace {
		template<typename TTraits, VersionType Version>
		void AssertCanPublishBalanceCreditNotifications() {
			// Arrange:
			mocks::MockTypedNotificationSubscriber<BalanceCreditNotification<1>> sub;
			test::MutableBlockchainConfiguration config;
			auto pPlugin = TTraits::CreatePlugin(config.Immutable);
			auto pTransaction = CreateTransaction<TTraits, Version>();
			auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(config.Immutable);

			// Act:
			test::PublishTransaction(*pPlugin, *pTransaction, sub);

			// Assert:
			ASSERT_EQ(2u, sub.numMatchingNotifications());

			const auto& notification1 = sub.matchingNotifications()[0];
			EXPECT_EQ(pTransaction->Signer, notification1.Sender);
			EXPECT_EQ(UnresolvedMosaicId(1), notification1.MosaicId);
			EXPECT_EQ(Amount(10), notification1.Amount);

			const auto& notification2 = sub.matchingNotifications()[1];
			EXPECT_EQ(pTransaction->Signer, notification2.Sender);
			EXPECT_EQ(currencyMosaicId, notification2.MosaicId);
			EXPECT_EQ(Amount(200), notification2.Amount);
		}
	}

	PLUGIN_TEST(CanPublishBalanceCreditNotifications_v1) {
		AssertCanPublishBalanceCreditNotifications<TTraits, 1>();
	}

	PLUGIN_TEST(CanPublishBalanceCreditNotifications_v2) {
		AssertCanPublishBalanceCreditNotifications<TTraits, 2>();
	}

	// endregion
}}
