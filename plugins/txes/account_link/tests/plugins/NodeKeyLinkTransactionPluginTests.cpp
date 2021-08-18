/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "src/plugins/NodeKeyLinkTransactionPlugin.h"
#include "src/model/AccountLinkNotifications.h"
#include "src/model/NodeKeyLinkTransaction.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/TestHarness.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS NodeKeyLinkTransactionPluginTests

	// region test utils

	namespace {
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(NodeKeyLink, 1, 1,)
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , Entity_Type_Node_Key_Link)

	// endregion

	// region publish - action link

	namespace {
		template<typename TTraits>
		void AddCommonExpectations(
				typename test::TransactionPluginTestUtils<TTraits>::PublishTestBuilder& builder,
				const typename TTraits::TransactionType& transaction) {
			builder.template addExpectation<KeyLinkActionNotification<1>>([&transaction](const auto& notification) {
				EXPECT_EQ(transaction.LinkAction, notification.LinkAction);
			});
			builder.template addExpectation<NodeAccountLinkNotification<1>>([&transaction](const auto& notification) {
				EXPECT_EQ(transaction.Signer, notification.MainAccountKey);
				EXPECT_EQ(transaction.RemoteAccountKey, notification.RemoteAccountKey);
				EXPECT_EQ(transaction.LinkAction, notification.LinkAction);
			});
		}
	}

	PLUGIN_TEST(CanPublishAllNotificationsInCorrectOrderWhenLinkActionIsLink) {
		// Arrange:
		typename TTraits::TransactionType transaction;
		transaction.LinkAction = AccountLinkAction::Link;
		transaction.Version = 0x1;
		transaction.Size = sizeof(transaction);

		// Act + Assert:
		test::TransactionPluginTestUtils<TTraits>::AssertNotificationTypes(transaction, {
			KeyLinkActionNotification<1>::Notification_Type,
			NodeAccountLinkNotification<1>::Notification_Type
		});
	}

	PLUGIN_TEST(CanPublishAllNotificationsWhenLinkActionIsLink) {
		// Arrange:
		typename TTraits::TransactionType transaction;
		transaction.LinkAction = AccountLinkAction::Link;
		transaction.Version = 0x1;
		transaction.Size = sizeof(transaction);

		typename test::TransactionPluginTestUtils<TTraits>::PublishTestBuilder builder;
		AddCommonExpectations<TTraits>(builder, transaction);

		// Act + Assert:
		builder.runTest(transaction);
	}

	// endregion

	// region publish - action unlink

	PLUGIN_TEST(CanPublishAllNotificationsInCorrectOrderWhenLinkActionIsUnlink) {
		// Arrange:
		typename TTraits::TransactionType transaction;
		transaction.LinkAction = AccountLinkAction::Unlink;
		transaction.Version = 0x1;
		transaction.Size = sizeof(transaction);

		// Act + Assert:
		test::TransactionPluginTestUtils<TTraits>::AssertNotificationTypes(transaction, {
			KeyLinkActionNotification<1>::Notification_Type,
			NodeAccountLinkNotification<1>::Notification_Type
		});
	}

	PLUGIN_TEST(CanPublishAllNotificationsWhenLinkActionIsUnlink) {
		// Arrange:
		typename TTraits::TransactionType transaction;
		transaction.LinkAction = AccountLinkAction::Unlink;
		transaction.Version = 0x1;
		transaction.Size = sizeof(transaction);

		typename test::TransactionPluginTestUtils<TTraits>::PublishTestBuilder builder;
		AddCommonExpectations<TTraits>(builder, transaction);

		// Act + Assert:
		builder.runTest(transaction);
	}

	// endregion
}}
