/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/utils/HexParser.h"
#include "src/plugins/EndDriveVerificationTransactionPlugin.h"
#include "src/model/EndDriveVerificationTransaction.h"
#include "src/model/InternalStorageNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/StorageTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS EndDriveVerificationTransactionPluginTests

        // region TransactionPlugin

        namespace {
            DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(EndDriveVerification, config::ImmutableConfiguration, 1, 1,)

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
                return test::CreateEndDriveVerificationTransaction<typename TTraits::TransactionType>();
            }
        }

        DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , Entity_Type_EndDriveVerification, CreateConfiguration())

        PLUGIN_TEST(CanCalculateSize) {
            // Arrange:
            auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
            auto pTransaction = CreateTransaction<TTraits>();
			pTransaction->ProversCount = 0;
			pTransaction->VerificationOpinionsCount = 0;

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
            ASSERT_EQ(1u, sub.numNotifications());
            auto i = 0u;
            EXPECT_EQ(Storage_End_Drive_Verification_v1_Notification, sub.notificationTypes()[i++]);
        }

        // endregion

        // region publish - end drive verification notification

        PLUGIN_TEST(CanPublishEndDriveVerificationNotification) {
            // Arrange:
            mocks::MockTypedNotificationSubscriber<EndDriveVerificationNotification<1>> sub;
            auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
            auto pTransaction = CreateTransaction<TTraits>();

            // Act:
            test::PublishTransaction(*pPlugin, *pTransaction, sub);

            // Assert:
            ASSERT_EQ(1u, sub.numMatchingNotifications());
            const auto& notification = sub.matchingNotifications()[0];
            EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
            EXPECT_EQ(pTransaction->VerificationTrigger, notification.VerificationTrigger);
        }

        // endregion
    }}
