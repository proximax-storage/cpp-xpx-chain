/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/EndExecuteTransactionPlugin.h"
#include "src/model/EndExecuteTransaction.h"
#include "src/model/SuperContractNotifications.h"
#include "plugins/txes/operation/src/model/OperationNotifications.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS EndExecuteTransactionPluginTests

	// region TransactionPlugin

	namespace {
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(EndExecute, 1, 1,)

		static const auto Generation_Hash = test::GenerateRandomByteArray<GenerationHash>();
		static constexpr auto Num_Mosaics = 5;

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateEndExecuteTransaction<typename TTraits::TransactionType>(Num_Mosaics);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS_ONLY_EMBEDDABLE(TEST_CLASS,,, Entity_Type_EndExecute)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();
		typename TTraits::TransactionType transaction;
		transaction.Size = sizeof(transaction);
		transaction.MosaicCount = Num_Mosaics;

		// Act:
		auto realSize = pPlugin->calculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + Num_Mosaics * sizeof(model::UnresolvedMosaic), realSize);
	}

	// endregion

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
		ASSERT_EQ(4u, sub.numNotifications());
		auto i = 0u;
		EXPECT_EQ(SuperContract_SuperContract_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(SuperContract_EndExecute_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Operation_Mosaic_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Operation_End_v1_Notification, sub.notificationTypes()[i++]);
	}

	// endregion

	// region super contract notification

	PLUGIN_TEST(CanPublishSuperContractNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<SuperContractNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.SuperContract);
	}

	// endregion

	// region end execute notification

	PLUGIN_TEST(CanPublishEndExecuteNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<EndExecuteNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.SuperContract);
	}

	// endregion

	// region operation mosaic notification

	PLUGIN_TEST(CanPublishOperationMosaicNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<OperationMosaicNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(Num_Mosaics, notification.MosaicCount);
		EXPECT_EQ_MEMORY(pTransaction->MosaicsPtr(), notification.MosaicsPtr, Num_Mosaics * sizeof(model::UnresolvedMosaic));
	}

	// endregion

	// region end operation notification

	PLUGIN_TEST(CanPublishEndOperationNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<EndOperationNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];

		EXPECT_EQ(pTransaction->Signer, notification.Executor);
		EXPECT_EQ(pTransaction->OperationToken, notification.OperationToken);
		EXPECT_EQ(Num_Mosaics, notification.MosaicCount);
		EXPECT_EQ_MEMORY(pTransaction->MosaicsPtr(), notification.MosaicsPtr, Num_Mosaics * sizeof(model::UnresolvedMosaic));
		EXPECT_EQ(pTransaction->Result, notification.Result);
	}

	// endregion
}}
