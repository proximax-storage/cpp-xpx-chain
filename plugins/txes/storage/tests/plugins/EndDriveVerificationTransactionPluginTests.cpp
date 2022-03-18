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

#include <boost/multiprecision/cpp_int.hpp>

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS EndDriveVerificationTransactionPluginTests

    // region TransactionPlugin

    namespace {
        DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(EndDriveVerification, config::ImmutableConfiguration, 1, 1,)

        const auto Generation_Hash = utils::ParseByteArray<GenerationHash>("CE076EF4ABFBC65B046987429E274EC31506D173E91BF102F16BEB7FB8176230");
        constexpr auto Network_Identifier = NetworkIdentifier::Mijin_Test;
		constexpr uint8_t Key_Count = 7;
		constexpr uint8_t Judging_Key_Count = 2;

        auto CreateConfiguration() {
            auto config = config::ImmutableConfiguration::Uninitialized();
            config.GenerationHash = Generation_Hash;
            config.NetworkIdentifier = Network_Identifier;
            return config;
        }

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateEndDriveVerificationTransaction<typename TTraits::TransactionType>(Key_Count, Judging_Key_Count);
		}
	}

    DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , Entity_Type_EndDriveVerification, CreateConfiguration())

	using BigInt = boost::multiprecision::uint256_t;

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

        // Act:
        auto realSize = pPlugin->calculateRealSize(*pTransaction);

        // Assert:
        EXPECT_EQ(sizeof(typename TTraits::TransactionType) + 354u, realSize);
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
        ASSERT_EQ(2u, sub.numNotifications());
        auto i = 0u;
        EXPECT_EQ(Storage_End_Drive_Verification_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Storage_Opinion_v1_Notification, sub.notificationTypes()[i++]);
    }

    // endregion

    // region publish - end drive verification notification
/*
    PLUGIN_TEST(CanPublishEndDriveVerificationNotification) {
        // Arrange:
        mocks::MockTypedNotificationSubscriber<EndDriveVerificationNotification<1>> sub;
        auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
        auto pTransaction = CreateTransaction<TTraits>();
		auto pPublicKeys1 = pTransaction->PublicKeysPtr();

        // Act:
        test::PublishTransaction(*pPlugin, *pTransaction, sub);

        // Assert:
        ASSERT_EQ(1u, sub.numMatchingNotifications());
        const auto& notification = sub.matchingNotifications()[0];
        EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
        EXPECT_EQ(pTransaction->VerificationTrigger, notification.VerificationTrigger);
        EXPECT_EQ(pTransaction->ShardId, notification.ShardId);
        EXPECT_EQ(pTransaction->KeyCount, notification.KeyCount);
        EXPECT_EQ(pTransaction->JudgingKeyCount, notification.JudgingKeyCount);
        EXPECT_EQ_MEMORY(pTransaction->PublicKeysPtr(), notification.PublicKeysPtr, Key_Count * Key_Size);
        EXPECT_EQ_MEMORY(pTransaction->SignaturesPtr(), notification.SignaturesPtr, Judging_Key_Count * Signature_Size);
        EXPECT_EQ_MEMORY(pTransaction->OpinionsPtr(), notification.OpinionsPtr, (Key_Count * Judging_Key_Count + 7u) / 8u);
    }
*/
    // endregion

    // region publish - opinion notification

    PLUGIN_TEST(CanPublishOpinionNotification) {
        // Arrange:
        mocks::MockTypedNotificationSubscriber<OpinionNotification<1>> sub;
        auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
        auto pTransaction = CreateTransaction<TTraits>();

        // Act:
        test::PublishTransaction(*pPlugin, *pTransaction, sub);

        // Assert:
        ASSERT_EQ(1u, sub.numMatchingNotifications());
        const auto& notification = sub.matchingNotifications()[0];
		auto commonDataSize = Key_Size + Hash256_Size + sizeof(uint16_t);
        EXPECT_EQ(commonDataSize, notification.CommonDataSize);
        EXPECT_EQ(0u, notification.JudgingKeysCount);
        EXPECT_EQ(Judging_Key_Count, notification.OverlappingKeysCount);
        EXPECT_EQ(Key_Count - Judging_Key_Count, notification.JudgedKeysCount);
        EXPECT_EQ(sizeof(uint8_t), notification.OpinionElementSize);
        EXPECT_EQ_MEMORY(pTransaction->DriveKey.data(), notification.CommonDataPtr, commonDataSize);
        EXPECT_EQ_MEMORY(pTransaction->PublicKeysPtr(), notification.PublicKeysPtr, Key_Count * Key_Size);
        EXPECT_EQ_MEMORY(pTransaction->SignaturesPtr(), notification.SignaturesPtr, Judging_Key_Count * Signature_Size);
        EXPECT_EQ_MEMORY(pTransaction->OpinionsPtr(), notification.OpinionsPtr, (Key_Count * Judging_Key_Count + 7u) / 8u);

		boost::dynamic_bitset<uint8_t> presentOpinions(Key_Count * Judging_Key_Count, uint16_t(-1));
		for (auto i = 0u; i < Judging_Key_Count; ++i)
			presentOpinions[i * (Key_Count + 1)] = 0;
		std::vector<uint8_t> buffer((Key_Count * Judging_Key_Count + 7u) / 8u, 0u);
		boost::to_block_range(presentOpinions, buffer.data());
		EXPECT_EQ_MEMORY(buffer.data(), notification.PresentOpinionsPtr, buffer.size());
    }

    // endregion
}}
