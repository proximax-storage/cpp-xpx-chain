/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/utils/MemoryUtils.h"
#include "src/plugins/RemoveExchangeOfferTransactionPlugin.h"
#include "src/model/RemoveExchangeOfferTransaction.h"
#include "src/model/ExchangeNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/ExchangeTestUtils.h"
#include "tests/TestHarness.h"
#include <limits>

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS RemoveExchangeOfferTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(RemoveExchangeOffer, 2, 2,)

		template<typename TTraits, VersionType Version>
		auto CreateTransaction() {
			return test::CreateExchangeTransaction<typename TTraits::TransactionType, model::OfferMosaic>(
				{
					model::OfferMosaic{UnresolvedMosaicId(1), model::OfferType::Sell},
					model::OfferMosaic{UnresolvedMosaicId(2), model::OfferType::Buy},
					model::OfferMosaic{UnresolvedMosaicId(3), model::OfferType::Sell},
					model::OfferMosaic{UnresolvedMosaicId(4), model::OfferType::Buy},
				},
				Version
			);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_Remove_Exchange_Offer)

	namespace {
		template<typename TTraits, VersionType Version>
		void AssertCanCalculateSize() {
			// Arrange:
			auto pPlugin = TTraits::CreatePlugin();
			auto pTransaction = CreateTransaction<TTraits, Version>();

			// Act:
			auto realSize = pPlugin->calculateRealSize(*pTransaction);

			// Assert:
			EXPECT_EQ(sizeof(typename TTraits::TransactionType) + 4 * sizeof(model::OfferMosaic), realSize);
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
		auto pPlugin = TTraits::CreatePlugin();

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
			auto pPlugin = TTraits::CreatePlugin();

			// Act:
			test::PublishTransaction(*pPlugin, *pTransaction, sub);

			// Assert:
			ASSERT_EQ(1, sub.numNotifications());
			EXPECT_EQ(expectedExchangeOfferType, sub.notificationTypes()[0]);
		}
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotifications_v1) {
		AssertCanPublishCorrectNumberOfNotifications<TTraits, 1>(Exchange_Remove_Offer_v1_Notification);
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotifications_v2) {
		AssertCanPublishCorrectNumberOfNotifications<TTraits, 2>(Exchange_Remove_Offer_v2_Notification);
	}

	// endregion

	// region publish - remove offer notification

	namespace {
		template<typename TTraits, VersionType Version>
		void AssertCanPublishRemoveOfferNotification() {
			// Arrange:
			mocks::MockTypedNotificationSubscriber<RemoveOfferNotification<Version>> sub;
			auto pPlugin = TTraits::CreatePlugin();
			auto pTransaction = CreateTransaction<TTraits, Version>();

			// Act:
			test::PublishTransaction(*pPlugin, *pTransaction, sub);

			// Assert:
			ASSERT_EQ(1u, sub.numMatchingNotifications());
			const auto& notification = sub.matchingNotifications()[0];
			EXPECT_EQ(pTransaction->Signer, notification.Owner);
			EXPECT_EQ(4, notification.OfferCount);
			EXPECT_EQ_MEMORY(pTransaction->OffersPtr(), notification.OffersPtr, 4 * sizeof(OfferMosaic));
		}
	}

	PLUGIN_TEST(CanPublishRemoveOfferNotification_v1) {
		AssertCanPublishRemoveOfferNotification<TTraits, 1>();
	}

	PLUGIN_TEST(CanPublishRemoveOfferNotification_v2) {
		AssertCanPublishRemoveOfferNotification<TTraits, 2>();
	}

	// endregion
}}
