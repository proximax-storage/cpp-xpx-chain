/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/EntityHasher.h"
#include "src/model/StartExecuteTransaction.h"
#include "src/model/SuperContractNotifications.h"
#include "tests/test/SuperContractTestUtils.h"
#include "src/plugins/StartExecuteTransactionPlugin.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "plugins/txes/operation/src/model/OperationNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS StartExecuteTransactionPluginTests

	// region TransactionPlugin

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(StartExecute, std::shared_ptr<config::BlockchainConfigurationHolder>, 1, 1,)

		static const auto Generation_Hash = test::GenerateRandomByteArray<GenerationHash>();
		static constexpr auto Operation_Duration = utils::BlockSpan::FromHours(1);
		static constexpr auto Block_Generation_Target_Time = utils::TimeSpan::FromSeconds(15);
		static constexpr auto Num_Mosaics = 5;
		static constexpr auto Function_Size = 10;
		static constexpr auto Data_Size = 15;

		auto CreateConfiguration() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.GenerationHash = Generation_Hash;
			config.Network.BlockGenerationTargetTime = Block_Generation_Target_Time;
			auto pluginConfig = config::OperationConfiguration::Uninitialized();
			pluginConfig.MaxOperationDuration = Operation_Duration;
			config.Network.SetPluginConfiguration(pluginConfig);
			return config.ToConst();
		}

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateStartExecuteTransaction<typename TTraits::TransactionType>(Num_Mosaics, Function_Size, Data_Size);
		}

		auto CalculateHash(const StartExecuteTransaction& transaction, const GenerationHash& generationHash, model::NotificationSubscriber&) {
			return model::CalculateHash(transaction, generationHash);
		}

		auto CalculateHash(const EmbeddedStartExecuteTransaction& transaction, const GenerationHash& generationHash, model::NotificationSubscriber& sub) {
			return model::CalculateHash(plugins::ConvertEmbeddedTransaction(transaction, Timestamp(), sub.mempool()), generationHash);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS_ONLY_EMBEDDABLE(
		TEST_CLASS,
		,
		,Entity_Type_StartExecute,
		config::CreateMockConfigurationHolder(CreateConfiguration()))

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));
		typename TTraits::TransactionType transaction;
		transaction.Size = sizeof(transaction);
		transaction.MosaicCount = Num_Mosaics;
		transaction.FunctionSize = Function_Size;
		transaction.DataSize = Data_Size;

		// Act:
		auto realSize = pPlugin->calculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + Num_Mosaics * sizeof(model::UnresolvedMosaic) + Function_Size + Data_Size, realSize);
	}

	// endregion

	// region publish - basic

	PLUGIN_TEST(PublishesNoNotificationWhenTransactionVersionIsInvalid) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));

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
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(5u + Num_Mosaics, sub.numNotifications());
		auto i = 0u;
		EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(SuperContract_SuperContract_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(SuperContract_StartExecute_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Operation_Mosaic_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Operation_Start_v1_Notification, sub.notificationTypes()[i++]);
		for (; i < 2u + Num_Mosaics; ++i)
			EXPECT_EQ(Core_Balance_Debit_v1_Notification, sub.notificationTypes()[i]);
	}

	// endregion

	// region super contract notification

	PLUGIN_TEST(CanPublishSuperContractNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<SuperContractNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->SuperContract, notification.SuperContract);
	}

	// endregion

	// region start execute notification

	PLUGIN_TEST(CanPublishStartExecuteNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<StartExecuteNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->SuperContract, notification.SuperContract);
	}

	// endregion

	// region operation mosaic notification

	PLUGIN_TEST(CanPublishOperationMosaicNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<OperationMosaicNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));
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
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];

		auto operationToken = CalculateHash(*pTransaction, Generation_Hash, sub);
		EXPECT_EQ(operationToken, notification.OperationToken);
		EXPECT_EQ(pTransaction->Signer, notification.Signer);
		auto duration = Operation_Duration.blocks(Block_Generation_Target_Time);
		EXPECT_EQ(duration, notification.Duration);
		EXPECT_EQ(Num_Mosaics, notification.MosaicCount);
		EXPECT_EQ_MEMORY(pTransaction->MosaicsPtr(), notification.MosaicsPtr, Num_Mosaics * sizeof(model::UnresolvedMosaic));
	}

	// endregion

	// region balance debit notifications

	PLUGIN_TEST(CanPublishBalanceDebitNotifications) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<BalanceDebitNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(Num_Mosaics, sub.numMatchingNotifications());

		for (auto i = 0u; i < Num_Mosaics; ++i) {
			const auto& notification = sub.matchingNotifications()[i];
			EXPECT_EQ(pTransaction->Signer, notification.Sender);
			EXPECT_EQ(test::UnresolveXor(MosaicId(i + 1)), notification.MosaicId);
			EXPECT_EQ(Amount((i + 1) * 10), notification.Amount);
		}
	}

	// endregion
}}
