/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/utils/MemoryUtils.h"
#include "src/plugins/RemoveSdaExchangeOfferTransactionPlugin.h"
#include "src/model/RemoveSdaExchangeOfferTransaction.h"
#include "src/model/SdaExchangeNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/SdaExchangeTestUtils.h"
#include "tests/TestHarness.h"
#include <limits>

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS RemoveSdaExchangeOfferTransactionPluginTests

    namespace {
        DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(RemoveSdaExchangeOffer, 1, 1,)

        template<typename TTraits, VersionType Version>
        auto CreateTransaction() {
            return test::CreatePlaceSdaExchangeOfferTransaction<typename TTraits::TransactionType, model::SdaOfferMosaic>(
                {
                    model::SdaOfferMosaic{UnresolvedMosaicId(1), UnresolvedMosaicId(2)},
                    model::SdaOfferMosaic{UnresolvedMosaicId(2), UnresolvedMosaicId(1)},
                },
                Version
            );
        }
    }

    DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_Remove_Sda_Exchange_Offer)

    namespace {
        template<typename TTraits, VersionType Version>
        void AssertCanCalculateSize() {
            // Arrange:
            auto pPlugin = TTraits::CreatePlugin();
            auto pTransaction = CreateTransaction<TTraits, Version>();

            // Act:
            auto realSize = pPlugin->calculateRealSize(*pTransaction);

            // Assert:
            EXPECT_EQ(sizeof(typename TTraits::TransactionType) + 2 * sizeof(model::SdaOfferMosaic), realSize);
        }
    }

    PLUGIN_TEST(CanCalculateSize_v1) {
        AssertCanCalculateSize<TTraits, 1>();
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
        AssertCanPublishCorrectNumberOfNotifications<TTraits, 1>(ExchangeSda_Remove_Sda_Offer_v1_Notification);
    }

    // endregion

    // region publish - remove offer notification

    namespace {
        template<typename TTraits, VersionType Version>
        void AssertCanPublishRemoveSdaOfferNotification() {
            // Arrange:
            mocks::MockTypedNotificationSubscriber<RemoveSdaOfferNotification<Version>> sub;
            auto pPlugin = TTraits::CreatePlugin();
            auto pTransaction = CreateTransaction<TTraits, Version>();

            // Act:
            test::PublishTransaction(*pPlugin, *pTransaction, sub);

            // Assert:
            ASSERT_EQ(1u, sub.numMatchingNotifications());
            const auto& notification = sub.matchingNotifications()[0];
            EXPECT_EQ(pTransaction->Signer, notification.Owner);
            EXPECT_EQ(2, notification.SdaOfferCount);
            EXPECT_EQ_MEMORY(pTransaction->SdaOffersPtr(), notification.SdaOffersPtr, 2 * sizeof(SdaOfferMosaic));
        }
    }

    PLUGIN_TEST(CanPublishRemoveSdaOfferNotification_v1) {
        AssertCanPublishRemoveSdaOfferNotification<TTraits, 1>();
    }

    // endregion
}