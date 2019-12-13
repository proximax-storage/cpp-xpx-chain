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
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(RemoveExchangeOffer, 1, 1,)

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateExchangeTransaction<typename TTraits::TransactionType, model::OfferMosaic>(
				{
					model::OfferMosaic{UnresolvedMosaicId(1), model::OfferType::Sell},
					model::OfferMosaic{UnresolvedMosaicId(2), model::OfferType::Buy},
					model::OfferMosaic{UnresolvedMosaicId(3), model::OfferType::Sell},
					model::OfferMosaic{UnresolvedMosaicId(4), model::OfferType::Buy},
				}
			);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_Remove_Exchange_Offer)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		auto realSize = pPlugin->calculateRealSize(*pTransaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + 4 * sizeof(model::OfferMosaic), realSize);
	}

	// region publish - basic

	PLUGIN_TEST(PublishesNoNotificationWhenTransactionVersionIsInvalid) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin();

		typename TTraits::TransactionType transaction;
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
		auto pPlugin = TTraits::CreatePlugin();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1, sub.numNotifications());
		EXPECT_EQ(Exchange_Remove_Offer_v1_Notification, sub.notificationTypes()[0]);
	}

	// endregion

	// region publish - remove offer notification

	PLUGIN_TEST(CanPublishRemoveOfferNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<RemoveOfferNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.Owner);
		EXPECT_EQ(4, notification.OfferCount);
		EXPECT_EQ(pTransaction->OffersPtr(), notification.OffersPtr);
	}

	// endregion
}}
