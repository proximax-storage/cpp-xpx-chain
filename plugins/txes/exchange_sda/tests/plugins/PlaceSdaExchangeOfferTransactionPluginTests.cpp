/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/utils/MemoryUtils.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "src/plugins/PlaceSdaExchangeOfferTransactionPlugin.h"
#include "src/model/PlaceSdaExchangeOfferTransaction.h"
#include "src/model/SdaExchangeNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/SdaExchangeTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"
#include <limits>

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS PlaceSdaExchangeOfferTransactionPluginTests

    namespace {
        DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(PlaceSdaExchangeOffer, config::ImmutableConfiguration, 1, 1,)

        constexpr auto Sda_Offer_Count = 4u;

        template<typename TTraits, VersionType Version>
        auto CreateTransaction(const Key& offerOwner1 = test::GenerateRandomByteArray<Key>(), const Key& offerOwner2 = test::GenerateRandomByteArray<Key>()) {
            return test::CreatePlaceSdaExchangeOfferTransaction<typename TTraits::TransactionType, model::SdaOfferWithOwnerAndDuration>(
                {
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{UnresolvedMosaicId(1), Amount(10)}, {UnresolvedMosaicId(2), Amount(100)}}, offerOwner1, BlockDuration(1000)},
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{UnresolvedMosaicId(2), Amount(200)}, {UnresolvedMosaicId(1), Amount(20)}}, offerOwner2, BlockDuration(1000)},
                },
                Version
            );
        }
    }

    DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_Place_Sda_Exchange_Offer, test::MutableBlockchainConfiguration().ToConst().Immutable)

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
            EXPECT_EQ(sizeof(typename TTraits::TransactionType) + Sda_Offer_Count * sizeof(model::SdaOfferWithOwnerAndDuration), realSize);
        }
    }

    PLUGIN_TEST(CanCalculateSize_v1) {
        AssertCanCalculateSize<TTraits, 1>();
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
        void AssertCanPublishCorrectNumberOfNotifications(NotificationType expectedPlaceSdaExchangeOfferType) {
            // Arrange:
            auto pTransaction = CreateTransaction<TTraits, Version>();
            mocks::MockNotificationSubscriber sub;
            test::MutableBlockchainConfiguration config;
            auto pPlugin = TTraits::CreatePlugin(config.Immutable);

            // Act:
            test::PublishTransaction(*pPlugin, *pTransaction, sub);

            // Assert:
            ASSERT_EQ(1 + Sda_Offer_Count * 2, sub.numNotifications());
            EXPECT_EQ(expectedPlaceSdaExchangeOfferType, sub.notificationTypes()[0]);
            for (auto i = 0u; i < Sda_Offer_Count; ++i) {
                EXPECT_EQ(Core_Balance_Debit_v1_Notification, sub.notificationTypes()[2 * i + 1]);
                EXPECT_EQ(Core_Balance_Credit_v1_Notification, sub.notificationTypes()[2 * i + 2]);
            }
        }
    }

    PLUGIN_TEST(CanPublishCorrectNumberOfNotifications_v1) {
        AssertCanPublishCorrectNumberOfNotifications<TTraits, 1>(ExchangeSda_Place_Sda_Offer_v1_Notification);
    }

    // endregion

    // region publish - place offer notification

    namespace {
        template<typename TTraits, VersionType Version>
        void AssertCanPublishPlaceSdaOfferNotification() {
            // Arrange:
            mocks::MockTypedNotificationSubscriber<PlaceSdaOfferNotification<Version>> sub;
            test::MutableBlockchainConfiguration config;
            auto pPlugin = TTraits::CreatePlugin(config.Immutable);
            auto pTransaction = CreateTransaction<TTraits, Version>();

            // Act:
            test::PublishTransaction(*pPlugin, *pTransaction, sub);

            // Assert:
            ASSERT_EQ(1u, sub.numMatchingNotifications());
            const auto& notification = sub.matchingNotifications()[0];
            EXPECT_EQ(pTransaction->Signer, notification.Signer);
            EXPECT_EQ(Sda_Offer_Count, notification.SdaOfferCount);
            EXPECT_EQ_MEMORY(pTransaction->SdaOffersPtr(), notification.SdaOffersPtr, Sda_Offer_Count * sizeof(model::SdaOfferWithOwnerAndDuration));
        }
    }

    PLUGIN_TEST(CanPublishOfferNotification_v1) {
        AssertCanPublishPlaceSdaOfferNotification<TTraits, 1>();
    }

    // endregion

    // region publish - balance debit notifications

    namespace {
        template<typename TTraits, VersionType Version>
        void AssertCanPublishBalanceDebitNotifications() {
            // Arrange:
            mocks::MockTypedNotificationSubscriber<BalanceDebitNotification<1>> sub;
            test::MutableBlockchainConfiguration config;
            auto pPlugin = TTraits::CreatePlugin(config.Immutable);
            auto pTransaction = CreateTransaction<TTraits, Version>();

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
            EXPECT_EQ(UnresolvedMosaicId(3), notification2.MosaicId);
            EXPECT_EQ(Amount(30), notification2.Amount);
        }
    }

    PLUGIN_TEST(CanPublishBalanceDebitNotifications_v1) {
        AssertCanPublishBalanceDebitNotifications<TTraits, 1>();
    }

    // endregion

    // region publish - balance credit notifications

    namespace {
        template<typename TTraits, VersionType Version>
        void AssertCanPublishBalanceCreditNotifications() {
            // Arrange:
            mocks::MockTypedNotificationSubscriber<BalanceCreditNotification<1>> sub;
            test::MutableBlockchainConfiguration config;
            auto pPlugin = TTraits::CreatePlugin(config.Immutable);
            auto pTransaction = CreateTransaction<TTraits, Version>();

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
            EXPECT_EQ(UnresolvedMosaicId(3), notification2.MosaicId);
            EXPECT_EQ(Amount(30), notification2.Amount);
        }
    }

    PLUGIN_TEST(CanPublishBalanceCreditNotifications_v1) {
        AssertCanPublishBalanceCreditNotifications<TTraits, 1>();
    }

    // endregion
}}