/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/DeployContractTransactionPlugin.h"
#include "src/model/DeployContractTransaction.h"
#include "src/catapult/model/SupercontractNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/SuperContractTestUtils.h"
#include <tests/test/nodeps/TestConstants.h>
#include "catapult/utils/HexParser.h"
#include "catapult/model/StorageNotifications.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/model/LiquidityProviderNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS DeployContractTransactionPluginTests

	// region TransactionPlugin

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(DeployContract, config::ImmutableConfiguration, 1, 1,)

		const auto Generation_Hash = utils::ParseByteArray<GenerationHash>("CE076EF4ABFBC65B046987429E274EC31506D173E91BF102F16BEB7FB8176230");
		constexpr auto Network_Identifier = NetworkIdentifier::Mijin_Test;

		auto CreateConfiguration() {
			auto config = config::ImmutableConfiguration::Uninitialized();
			config.GenerationHash = Generation_Hash;
			config.NetworkIdentifier = Network_Identifier;
			config.StreamingMosaicId = test::Default_Streaming_Mosaic_Id;
			config.SuperContractMosaicId = test::Default_Super_Contract_Mosaic_Id;
			return config;
		}

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateDeployContractTransaction<typename TTraits::TransactionType>();
		}

		Hash256 CalculateTransactionHash(const model::Transaction& transaction,
										 const GenerationHash& generationHash,
										 model::NotificationSubscriber& sub) {
			return CalculateHash(transaction, generationHash);
		}

		Hash256 CalculateTransactionHash(const model::EmbeddedTransaction& transaction,
										 const GenerationHash& generationHash,
										 model::NotificationSubscriber& sub) {
			Timestamp deadline;
			return CalculateHash(ConvertEmbeddedTransaction(transaction, deadline, sub.mempool()), generationHash);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_DeployContractTransaction, CreateConfiguration())

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();
		pTransaction->ServicePaymentsCount = 0;
		auto actualSize = sizeof(typename TTraits::TransactionType) +
						  pTransaction->FileNameSize +
						  pTransaction->FunctionNameSize +
						  pTransaction->ActualArgumentsSize +
						  pTransaction->AutomaticExecutionFileNameSize +
						  pTransaction->AutomaticExecutionFunctionNameSize;

		// Act:
		auto realSize = pPlugin->calculateRealSize(*pTransaction);

		// Assert:
		EXPECT_EQ(actualSize, realSize);
	}

	//	 region publish - basic

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
//		pTransaction->ServicePaymentsCount = 1;

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(11u, sub.numNotifications());
		auto i = 0u;
		EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Storage_Owner_Management_Prohibition_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(SuperContract_v2_Deploy_Supercontract_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Balance_Debit_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(SuperContract_v2_Automatic_Executions_Replenishment_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(SuperContract_v2_Manual_Call_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(LiquidityProvider_Credit_Mosaic_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(LiquidityProvider_Credit_Mosaic_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(LiquidityProvider_Credit_Mosaic_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(LiquidityProvider_Credit_Mosaic_v1_Notification, sub.notificationTypes()[i++]);
	}

	//	 endregion

//	// region publish - deploy contract notification
//
//	PLUGIN_TEST(CanPublishDeployContractNotification) {
//		// Arrange:
//		mocks::MockTypedNotificationSubscriber<DeploySupercontractNotification<1>> sub;
//		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
//		auto pTransaction = CreateTransaction<TTraits>();
//		const std::string automaticExecutionsFileName(reinterpret_cast<const char*>(pTransaction->AutomaticExecutionFileNamePtr()), pTransaction->AutomaticExecutionFileNameSize);
//		const std::string automaticExecutionsFunctionName(reinterpret_cast<const char*>(pTransaction->AutomaticExecutionFunctionNamePtr()), pTransaction->AutomaticExecutionFunctionNameSize);
//
//		// Act:
//		test::PublishTransaction(*pPlugin, *pTransaction, sub);
//
//		// Assert:
//		ASSERT_EQ(1u, sub.numMatchingNotifications());
//		const auto& notification = sub.matchingNotifications()[0];
//		EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
//		EXPECT_EQ(pTransaction->Assignee, notification.Assignee);
//		EXPECT_EQ(automaticExecutionsFileName, notification.AutomaticExecutionFileName);
//		EXPECT_EQ(automaticExecutionsFunctionName, notification.AutomaticExecutionsFunctionName);
//		EXPECT_EQ(pTransaction->AutomaticExecutionCallPayment, notification.AutomaticExecutionCallPayment);
//		EXPECT_EQ(pTransaction->AutomaticDownloadCallPayment, notification.AutomaticDownloadCallPayment);
//	}
//
//	// endregion
//
//	// region publish - account public key notification
//
//	PLUGIN_TEST(CanPublishAccountPublicKeyNotification) {
//		// Arrange:
//		mocks::MockTypedNotificationSubscriber<AccountPublicKeyNotification<1>> sub;
//		const auto& config = CreateConfiguration();
//		auto pPlugin = TTraits::CreatePlugin(config);
//		auto pTransaction = CreateTransaction<TTraits>();
//		auto contractKey = Key(CalculateTransactionHash(*pTransaction, config.GenerationHash, sub).array());
//
//		// Act:
//		test::PublishTransaction(*pPlugin, *pTransaction, sub);
//
//		// Assert:
//		ASSERT_EQ(1u, sub.numMatchingNotifications());
//		const auto& notification = sub.matchingNotifications()[0];
//
//		EXPECT_EQ(contractKey, notification.PublicKey);
//	}
//
//	// endregion
//
//	// region publish - owner management prohibition notification
//
//	PLUGIN_TEST(CanPublishOwnerManagementProhibitionNotification) {
//		// Arrange:
//		mocks::MockTypedNotificationSubscriber<OwnerManagementProhibitionNotification<1>> sub;
//		const auto& config = CreateConfiguration();
//		auto pPlugin = TTraits::CreatePlugin(config);
//		auto pTransaction = CreateTransaction<TTraits>();
//
//		// Act:
//		test::PublishTransaction(*pPlugin, *pTransaction, sub);
//
//		// Assert:
//		ASSERT_EQ(1u, sub.numMatchingNotifications());
//		const auto& notification = sub.matchingNotifications()[0];
//
//		EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
//		EXPECT_EQ(pTransaction->Signer, notification.Signer);
//	}
//
//	// endregion
//
//	// region publish - balance debit notification
//
//	PLUGIN_TEST(CanPublishBalanceDebitNotification) {
//		// Arrange:
//		mocks::MockTypedNotificationSubscriber<BalanceDebitNotification<1>> sub;
//		const auto& config = CreateConfiguration();
//		auto pPlugin = TTraits::CreatePlugin(config);
//		auto pTransaction = CreateTransaction<TTraits>();
//		const auto* const pServicePayment = pTransaction->ServicePaymentsPtr();
//		std::vector<UnresolvedMosaic> servicePayments;
//		servicePayments.reserve(pTransaction->ServicePaymentsCount);
//		for (auto i = 0U; i < pTransaction->ServicePaymentsCount; i++) {
//			servicePayments.push_back(pServicePayment[i]);
//		}
//
//		// Act:
//		test::PublishTransaction(*pPlugin, *pTransaction, sub);
//
//		// Assert:
//		ASSERT_EQ(1u, sub.numMatchingNotifications());
//		const auto& notification = sub.matchingNotifications()[0];
//
//		EXPECT_EQ(pTransaction->Signer, notification.Sender);
//		for (auto i = 0U; i < pTransaction->ServicePaymentsCount; i++) {
//			EXPECT_EQ(servicePayments[i].MosaicId, notification.MosaicId);
//			EXPECT_EQ(servicePayments[i].Amount, notification.Amount);
//		}
//	}
//
//	// endregion
//
//	// region publish - automatic executions payment notification
//
//	PLUGIN_TEST(CanPublishAutomaticExecutionsPaymentNotification) {
//		// Arrange:
//		mocks::MockTypedNotificationSubscriber<AutomaticExecutionsReplenishmentNotification<1>> sub;
//		const auto& config = CreateConfiguration();
//		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
//		auto pTransaction = CreateTransaction<TTraits>();
//		auto contractKey = Key(CalculateTransactionHash(*pTransaction, config.GenerationHash, sub).array());
//
//		// Act:
//		test::PublishTransaction(*pPlugin, *pTransaction, sub);
//
//		// Assert:
//		ASSERT_EQ(1u, sub.numMatchingNotifications());
//		const auto& notification = sub.matchingNotifications()[0];
//		EXPECT_EQ(contractKey, notification.ContractKey);
//		EXPECT_EQ(pTransaction->AutomaticExecutionsNumber, notification.Number);
//	}
//
//	// endregion
//
//	// region publish - manual call notification
//
//	PLUGIN_TEST(CanPublishManualCallNotification) {
//		// Arrange:
//		mocks::MockTypedNotificationSubscriber<ManualCallNotification<1>> sub;
//		const auto& config = CreateConfiguration();
//		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
//		auto pTransaction = CreateTransaction<TTraits>();
//		auto txHash = CalculateTransactionHash(*pTransaction, config.GenerationHash, sub);
//		auto contractKey = Key(txHash.array());
//		const std::string initializerFileName(reinterpret_cast<const char*>(pTransaction->FileNamePtr()), pTransaction->FileNameSize);
//		const std::string initializerFunctionName(reinterpret_cast<const char*>(pTransaction->FunctionNamePtr()), pTransaction->FunctionNameSize);
//		const std::string initializerActualArguments(reinterpret_cast<const char*>(pTransaction->ActualArgumentsPtr()), pTransaction->ActualArgumentsSize);
//
//		// Act:
//		test::PublishTransaction(*pPlugin, *pTransaction, sub);
//
//		// Assert:
//		ASSERT_EQ(1u, sub.numMatchingNotifications());
//		const auto& notification = sub.matchingNotifications()[0];
//		EXPECT_EQ(contractKey, notification.ContractKey);
//		EXPECT_EQ(txHash, notification.CallId);
//		EXPECT_EQ(pTransaction->Signer, notification.Caller);
//		EXPECT_EQ(initializerFileName, notification.FileName);
//		EXPECT_EQ(initializerFunctionName, notification.FunctionName);
//		EXPECT_EQ(initializerActualArguments, notification.ActualArguments);
//		EXPECT_EQ(pTransaction->ExecutionCallPayment, notification.ExecutionCallPayment);
//		EXPECT_EQ(pTransaction->DownloadCallPayment, notification.DownloadCallPayment);
//	}
//
//	// endregion
//
//	// region publish - credit mosaic notification
//
//	PLUGIN_TEST(CanPublishCreditMosaicNotification) {
//		// Arrange:
//		mocks::MockTypedNotificationSubscriber<CreditMosaicNotification<1>> sub;
//		const auto& config = CreateConfiguration();
//		auto pPlugin = TTraits::CreatePlugin(config);
//		auto pTransaction = CreateTransaction<TTraits>();
//		auto contractKey = Key(CalculateTransactionHash(*pTransaction, config.GenerationHash, sub).array());
//		Hash256 paymentHash;
//		crypto::Sha3_256_Builder sha3;
//		sha3.update({contractKey, config.GenerationHash});
//		sha3.final(paymentHash);
//		auto contractExecutionPaymentKey = Key(paymentHash.array());
//
//		// Act:
//		test::PublishTransaction(*pPlugin, *pTransaction, sub);
//
//		// Assert:
//		ASSERT_EQ(2u, sub.numMatchingNotifications());
//		const auto& notification1 = sub.matchingNotifications()[0];
//
//		EXPECT_EQ(pTransaction->Signer, notification1.CurrencyDebtor);
//		EXPECT_EQ(contractExecutionPaymentKey, notification1.MosaicCreditor);
//		EXPECT_EQ(notification1.MosaicId.unwrap(), config.SuperContractMosaicId.unwrap());
//		EXPECT_EQ(notification1.MosaicAmount.unwrap(), 0u);
//
//		const auto& notification2 = sub.matchingNotifications()[0];
//		EXPECT_EQ(pTransaction->Signer, notification2.CurrencyDebtor);
//		EXPECT_EQ(contractExecutionPaymentKey, notification2.MosaicCreditor);
//		EXPECT_EQ(notification2.MosaicId.unwrap(), config.StreamingMosaicId.unwrap());
//		EXPECT_EQ(notification2.MosaicAmount.unwrap(), 0u);
//
//		const auto& notification3 = sub.matchingNotifications()[1];
//		EXPECT_EQ(pTransaction->Signer, notification3.CurrencyDebtor);
//		EXPECT_EQ(contractExecutionPaymentKey, notification3.MosaicCreditor);
//		EXPECT_EQ(notification3.MosaicId.unwrap(), config.SuperContractMosaicId.unwrap());
//		EXPECT_EQ(notification3.MosaicAmount.unwrap(), 0u);
//
//		const auto& notification4 = sub.matchingNotifications()[2];
//		EXPECT_EQ(pTransaction->Signer, notification4.CurrencyDebtor);
//		EXPECT_EQ(contractExecutionPaymentKey, notification4.MosaicCreditor);
//		EXPECT_EQ(notification4.MosaicId.unwrap(), config.StreamingMosaicId.unwrap());
//		EXPECT_EQ(notification4.MosaicAmount.unwrap(), 0u);
//	}
//
//	// endregion
}}