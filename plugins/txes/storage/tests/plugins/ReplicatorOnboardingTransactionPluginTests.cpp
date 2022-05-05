/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/ReplicatorOnboardingTransactionPlugin.h"
#include "src/model/ReplicatorOnboardingTransaction.h"
#include "src/catapult/model/StorageNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/StorageTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS ReplicatorOnboardingTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(ReplicatorOnboarding, config::ImmutableConfiguration, 1, 1,)

		auto CreateConfiguration() {
			auto config = config::ImmutableConfiguration::Uninitialized();
			// config.GenerationHash = Generation_Hash;
			// config.NetworkIdentifier = Network_Identifier;
			return config;
		}

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateReplicatorOnboardingTransaction<typename TTraits::TransactionType>();
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_ReplicatorOnboarding, CreateConfiguration())

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

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
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
        ASSERT_EQ(7u, sub.numNotifications());
        auto i = 0u;
		EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Storage_Replicator_Onboarding_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Balance_Debit_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Balance_Credit_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Balance_Debit_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Balance_Credit_v1_Notification, sub.notificationTypes()[i++]);
	}

	// endregion

	// region publish - replicator onboarding notification

	PLUGIN_TEST(CanPublishReplicatorOnboardingNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ReplicatorOnboardingNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
        EXPECT_EQ(pTransaction->Capacity, notification.Capacity);
	}

	// endregion
}}
