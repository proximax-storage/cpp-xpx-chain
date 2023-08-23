/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/AddDbrbProcessTransactionPlugin.h"
#include "src/model/AddDbrbProcessTransaction.h"
#include "src/model/DbrbNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/DbrbTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS AddDbrbProcessTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(AddDbrbProcess, 1, 1,)

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateAddDbrbProcessTransaction<typename TTraits::TransactionType>();
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_AddDbrbProcess)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();

		typename TTraits::TransactionType transaction;
		transaction.Size = 0;

		// Act:
		auto realSize = pPlugin->calculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + transaction.HarvesterKeysCount * Key_Size, realSize);
	}

	// region publish - basic

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

	PLUGIN_TEST(CanPublishCorrectNumberOfNotifications) {
		// Arrange:
		auto pTransaction = CreateTransaction<TTraits>();
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(2, sub.numNotifications());
		EXPECT_EQ(Dbrb_AddDbrbProcess_v1_Notification, sub.notificationTypes()[0]);
	}

	// endregion

	// region publish - add harvester notification

	PLUGIN_TEST(CanPublishAddDbrbProcessNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<AddDbrbProcessNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();

		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];

		EXPECT_EQ(pTransaction->Signer, notification.ProcessId);
	}

	// endregion
}}
