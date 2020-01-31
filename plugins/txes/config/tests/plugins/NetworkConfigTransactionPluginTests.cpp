/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/utils/MemoryUtils.h"
#include "src/plugins/NetworkConfigTransactionPlugin.h"
#include "src/model/NetworkConfigTransaction.h"
#include "src/model/NetworkConfigNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/TestHarness.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS NetworkConfigTransactionPluginTests

	namespace {
		constexpr auto Transaction_Version = MakeVersion(NetworkIdentifier::Mijin_Test, 1);

		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(NetworkConfig, 1, 1,)

		template<typename TTraits>
		auto CreateTransactionWithAttachments(uint16_t networkConfigSize, uint16_t supportedEntityVersionsSize) {
			using TransactionType = typename TTraits::TransactionType;
			uint32_t entitySize = sizeof(TransactionType) + networkConfigSize + supportedEntityVersionsSize;
			auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
			pTransaction->Version = Transaction_Version;
			pTransaction->Size = entitySize;
			pTransaction->BlockChainConfigSize = networkConfigSize;
			pTransaction->SupportedEntityVersionsSize = supportedEntityVersionsSize;
			test::FillWithRandomData(pTransaction->Signer);
			pTransaction->Type = static_cast<model::EntityType>(0x4159);
			return pTransaction;
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_Network_Config)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();

		typename TTraits::TransactionType transaction;
		transaction.Size = 0;
		transaction.BlockChainConfigSize = 7;
		transaction.SupportedEntityVersionsSize = 8;

		// Act:
		auto realSize = pPlugin->calculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + 7 + 8, realSize);
	}

	PLUGIN_TEST(PublishesNoNotificationWhenTransactionVersionIsInvalid) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin();

		typename TTraits::TransactionType transaction;
		transaction.Size = sizeof(transaction);
		transaction.Version = MakeVersion(NetworkIdentifier::Mijin_Test, std::numeric_limits<uint32_t>::max());

		// Act:
		test::PublishTransaction(*pPlugin, transaction, sub);

		// Assert:
		ASSERT_EQ(0, sub.numNotifications());
	}

	// region publish - basic

	namespace {
		template<typename TTraits>
		void AssertNumNotifications(
				size_t numExpectedNotifications,
				const typename TTraits::TransactionType& transaction) {
			// Arrange:
			mocks::MockNotificationSubscriber sub;
			auto pPlugin = TTraits::CreatePlugin();

			// Act:
			test::PublishTransaction(*pPlugin, transaction, sub);

			// Assert:
			ASSERT_EQ(numExpectedNotifications, sub.numNotifications());
			EXPECT_EQ(NetworkConfig_Signer_v1_Notification, sub.notificationTypes()[0]);
			EXPECT_EQ(NetworkConfig_Network_Config_v1_Notification, sub.notificationTypes()[1]);
		}
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotificationsWhenNoAttachmentsArePresent) {
		// Arrange:
		auto pTransaction = CreateTransactionWithAttachments<TTraits>(0, 0);

		// Assert:
		AssertNumNotifications<TTraits>(2, *pTransaction);
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotificationsWhenBlockChainConfigNotPresent) {
		// Arrange:
		auto pTransaction = CreateTransactionWithAttachments<TTraits>(0, 10);

		// Assert:
		AssertNumNotifications<TTraits>(2, *pTransaction);
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotificationsWhenSupportedEntityVersionsNotPresent) {
		// Arrange:
		auto pTransaction = CreateTransactionWithAttachments<TTraits>(10, 0);

		// Assert:
		AssertNumNotifications<TTraits>(2, *pTransaction);
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotificationsWhenAttachmentsArePresent) {
		// Arrange:
		auto pTransaction = CreateTransactionWithAttachments<TTraits>(10, 20);

		// Assert:
		AssertNumNotifications<TTraits>(2, *pTransaction);
	}

	// endregion

	// region publish - network config notification

	PLUGIN_TEST(CanPublishNetworkConfigNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<NetworkConfigNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();

		auto pTransaction = CreateTransactionWithAttachments<TTraits>(10, 20);
		pTransaction->ApplyHeightDelta = BlockDuration{100};

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(100, notification.ApplyHeightDelta.unwrap());
		EXPECT_EQ(10, notification.BlockChainConfigSize);
		EXPECT_EQ(20, notification.SupportedEntityVersionsSize);
	}

	// endregion

	// region publish - network config signer notification

	PLUGIN_TEST(CanPublishNetworkConfigSignerNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<NetworkConfigSignerNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();

		auto pTransaction = CreateTransactionWithAttachments<TTraits>(20, 10);
		pTransaction->Signer = test::GenerateRandomByteArray<Key>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.Signer);
	}

	// endregion
}}
