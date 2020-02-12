/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/EntityHasher.h"
#include "src/plugins/StartOperationTransactionPlugin.h"
#include "src/model/OperationNotifications.h"
#include "src/model/StartOperationTransaction.h"
#include "tests/test/OperationTestUtils.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "plugins/txes/aggregate/src/plugins/Common.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS StartOperationTransactionPluginTests

	// region TransactionPlugin

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(StartOperation, config::ImmutableConfiguration, 1, 1,)

		static const auto Generation_Hash = test::GenerateRandomByteArray<GenerationHash>();
		static constexpr auto Num_Mosaics = 5;
		static constexpr auto Num_Executors = 10;

		auto CreateConfiguration() {
			auto config = config::ImmutableConfiguration::Uninitialized();
			config.GenerationHash = Generation_Hash;
			return config;
		}

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateStartOperationTransaction<typename TTraits::TransactionType>(Num_Mosaics, Num_Executors);
		}

		auto CalculateHash(const StartOperationTransaction& transaction, const GenerationHash& generationHash, model::NotificationSubscriber&) {
			return model::CalculateHash(transaction, generationHash);
		}

		auto CalculateHash(const EmbeddedStartOperationTransaction& transaction, const GenerationHash& generationHash, model::NotificationSubscriber& sub) {
			return model::CalculateHash(plugins::ConvertEmbeddedTransaction(transaction, Timestamp(), sub.mempool()), generationHash);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(
		TEST_CLASS,
		,
		,Entity_Type_StartOperation,
		CreateConfiguration())

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		typename TTraits::TransactionType transaction;
		transaction.Size = sizeof(transaction);
		transaction.MosaicCount = Num_Mosaics;
		transaction.ExecutorCount = Num_Executors;

		// Act:
		auto realSize = pPlugin->calculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + Num_Mosaics * sizeof(model::UnresolvedMosaic) + Num_Executors * Key_Size, realSize);
	}

	// endregion

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
		ASSERT_EQ(3u + Num_Mosaics, sub.numNotifications());
		auto i = 0u;
		EXPECT_EQ(Operation_Duration_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Operation_Mosaic_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Operation_Start_v1_Notification, sub.notificationTypes()[i++]);
		for (; i < 3u + Num_Mosaics; ++i)
			EXPECT_EQ(Core_Balance_Debit_v1_Notification, sub.notificationTypes()[i]);
	}

	// endregion

	// region operation duration notification

	PLUGIN_TEST(CanPublishOperationDurationNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<OperationDurationNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Duration, notification.Duration);
	}

	// endregion

	// region operation mosaic notification

	PLUGIN_TEST(CanPublishOperationMosaicNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<OperationMosaicNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
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

	// region start operation notification

	PLUGIN_TEST(CanPublishStartOperationNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<StartOperationNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];

		auto operationToken = CalculateHash(*pTransaction, Generation_Hash, sub);
		EXPECT_EQ(operationToken, notification.OperationToken);
		EXPECT_EQ(pTransaction->Signer, notification.Signer);
		EXPECT_EQ(pTransaction->Duration, notification.Duration);
		EXPECT_EQ(Num_Mosaics, notification.MosaicCount);
		EXPECT_EQ_MEMORY(pTransaction->MosaicsPtr(), notification.MosaicsPtr, Num_Mosaics * sizeof(model::UnresolvedMosaic));
	}

	// endregion

	// region publish - balance debit notifications

	PLUGIN_TEST(CanPublishBalanceDebitNotifications) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<BalanceDebitNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(Num_Mosaics, sub.numMatchingNotifications());

		for (auto i = 0u; i < Num_Mosaics; ++i) {
			const auto& notification = sub.matchingNotifications()[i];
			EXPECT_EQ(pTransaction->Signer, notification.Sender);
			EXPECT_EQ(UnresolvedMosaicId(i + 1), notification.MosaicId);
			EXPECT_EQ(Amount((i + 1) * 10), notification.Amount);
		}
	}

	// endregion
}}
