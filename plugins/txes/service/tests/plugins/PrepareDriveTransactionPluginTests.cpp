/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/PrepareDriveTransactionPlugin.h"
#include "src/model/PrepareDriveTransaction.h"
#include "src/model/ServiceNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/TestHarness.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS PrepareDriveTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(PrepareDrive, 1, 1,)

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreatePrepareDriveTransaction<typename TTraits::TransactionType>();
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS_ONLY_EMBEDDABLE(TEST_CLASS,,, Entity_Type_PrepareDrive)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();
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
		ASSERT_EQ(1, sub.numNotifications());
		EXPECT_EQ(Service_Prepare_Drive_v1_Notification, sub.notificationTypes()[0]);
	}

	// endregion

	// region publish - prepare drive notification

	PLUGIN_TEST(CanPublishPrepareDriveNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<PrepareDriveNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();

		auto pTransaction = CreateTransaction<TTraits>();
		pTransaction->Signer = test::GenerateRandomByteArray<Key>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.DriveKey);
		EXPECT_EQ(pTransaction->Owner, notification.Owner);
		EXPECT_EQ(pTransaction->Duration, notification.Duration);
		EXPECT_EQ(pTransaction->BillingPeriod, notification.BillingPeriod);
		EXPECT_EQ(pTransaction->BillingPrice, notification.BillingPrice);
		EXPECT_EQ(pTransaction->DriveSize, notification.DriveSize);
		EXPECT_EQ(pTransaction->Replicas, notification.Replicas);
		EXPECT_EQ(pTransaction->MinReplicators, notification.MinReplicators);
		EXPECT_EQ(pTransaction->PercentApprovers, notification.PercentApprovers);
	}

	// endregion
}}
