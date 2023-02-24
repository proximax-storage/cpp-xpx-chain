/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/UnsuccessfulEndBatchExecutionTransactionPlugin.h"
#include "src/model/UnsuccessfulEndBatchExecutionTransaction.h"
#include "src/model/InternalSuperContractNotifications.h"
#include "src/catapult/model/SupercontractNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/SuperContractTestUtils.h"
#include <tests/test/nodeps/TestConstants.h>
#include "catapult/utils/HexParser.h"

using namespace catapult::model;
namespace catapult { namespace plugins {

#define TEST_CLASS UnsuccessfulEndBatchExecutionTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(UnsuccessfulEndBatchExecution, config::ImmutableConfiguration, 1, 1, )

		const auto Generation_Hash = utils::ParseByteArray<GenerationHash>(
				"CE076EF4ABFBC65B046987429E274EC31506D173E91BF102F16BEB7FB8176230");
		constexpr auto Network_Identifier = NetworkIdentifier::Mijin_Test;

		auto CreateConfiguration() {
			auto config = config::ImmutableConfiguration::Uninitialized();
			config.GenerationHash = Generation_Hash;
			config.NetworkIdentifier = Network_Identifier;
			return config;
		}

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateUnsuccessfulEndBatchExecutionTransaction<typename TTraits::TransactionType>();
		}

		template<class T>
		void pushBytes(std::vector<uint8_t>& buffer, const T& data) {
			const auto* pBegin = reinterpret_cast<const uint8_t*>(&data);
			buffer.insert(buffer.end(), pBegin, pBegin + sizeof(data));
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_UnsuccessfulEndBatchExecutionTransaction, CreateConfiguration())

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();
		auto actualSize = sizeof(typename TTraits::TransactionType) +
						  pTransaction->CosignersNumber * sizeof(Key) +
						  pTransaction->CosignersNumber * sizeof(Signature) +
						  pTransaction->CosignersNumber * sizeof(RawProofOfExecution) +
						  pTransaction->CallsNumber * sizeof(ShortCallDigest) +
						  static_cast<uint64_t>(pTransaction->CosignersNumber) * static_cast<uint64_t>(pTransaction->CallsNumber) * sizeof(CallPayment);


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

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(6u, sub.numNotifications());
		auto i = 0u;
		EXPECT_EQ(SuperContract_v2_Opinion_Signature_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(SuperContract_v2_Contract_State_Update_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(SuperContract_v2_End_Batch_Execution_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(SuperContract_v2_Unsuccessful_Batch_Execution_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(SuperContract_v2_Batch_Calls_Notification_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(SuperContract_v2_Proof_Of_Execution_v1_Notification, sub.notificationTypes()[i++]);
	}

	// endregion

	// region publish - unsuccessful end batch execution notification

	PLUGIN_TEST(CanPublishUnsuccessfulEndBatchExecutionNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<UnsuccessfulBatchExecutionNotification<1>> sub;
		const auto& config = CreateConfiguration();
		auto pPlugin = TTraits::CreatePlugin(config);
		auto pTransaction = CreateTransaction<TTraits>();
		std::set<Key> cosigners;
		for (uint i = 0; i < pTransaction->CosignersNumber; i++) {
			cosigners.insert(pTransaction->PublicKeysPtr()[i]);
		}

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->ContractKey, notification.ContractKey);
		EXPECT_EQ(pTransaction->BatchId, notification.BatchId);
		EXPECT_EQ(cosigners, notification.Cosigners);
	}

	// endregion

	// region publish - opinion signature notification

	PLUGIN_TEST(CanPublishOpinionSignatureNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<OpinionSignatureNotification<1>> sub;
		const auto& config = CreateConfiguration();
		auto pPlugin = TTraits::CreatePlugin(config);
		auto pTransaction = CreateTransaction<TTraits>();
		std::vector<uint8_t> commonData;
		pushBytes(commonData, pTransaction->ContractKey);
		pushBytes(commonData, pTransaction->BatchId);
		pushBytes(commonData, pTransaction->AutomaticExecutionsNextBlockToCheck);
		for (uint j = 0; j < pTransaction->CallsNumber; j++) {
			pushBytes(commonData, pTransaction->CallDigestsPtr()[j]);
		}
		std::vector<model::Opinion> opinions;
		opinions.reserve(pTransaction->CosignersNumber);
		for (uint i = 0; i < pTransaction->CosignersNumber; i++) {
			std::vector<uint8_t> data;
			const model::RawProofOfExecution& proofOfExecution = pTransaction->ProofsOfExecutionPtr()[i];
			pushBytes(data, proofOfExecution);
			for (uint j = 0; j < pTransaction->CallsNumber; j++) {
				pushBytes(data, pTransaction->CallPaymentsPtr()[i * pTransaction->CallsNumber + j]);
			}
			opinions.emplace_back(pTransaction->PublicKeysPtr()[i], pTransaction->SignaturesPtr()[i], std::move(data));
		}

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(commonData, notification.CommonData);
		for (uint i = 0; i < pTransaction->CosignersNumber; i++) {
			EXPECT_EQ(opinions[i].PublicKey, notification.Opinions[i].PublicKey);
			EXPECT_EQ(opinions[i].Data, notification.Opinions[i].Data);
			EXPECT_EQ(opinions[i].Sign, notification.Opinions[i].Sign);
		}
	}

	// endregion

	// region publish - contract state update notification

	PLUGIN_TEST(CanContractStateUpdateNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ContractStateUpdateNotification<1>> sub;
		const auto& config = CreateConfiguration();
		auto pPlugin = TTraits::CreatePlugin(config);
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];

		EXPECT_EQ(pTransaction->ContractKey, notification.ContractKey);
	}

	// endregion

	// region publish - end batch execution notification

	PLUGIN_TEST(CanEndBatchExecutionNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<EndBatchExecutionNotification<1>> sub;
		const auto& config = CreateConfiguration();
		auto pPlugin = TTraits::CreatePlugin(config);
		auto pTransaction = CreateTransaction<TTraits>();
		std::vector<Key> cosignersList;
		for (uint i = 0; i < pTransaction->CosignersNumber; i++) {
			cosignersList.push_back(pTransaction->PublicKeysPtr()[i]);
		}

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];

		EXPECT_EQ(pTransaction->ContractKey, notification.ContractKey);
		EXPECT_EQ(pTransaction->BatchId, notification.BatchId);
		EXPECT_EQ(pTransaction->AutomaticExecutionsNextBlockToCheck, notification.AutomaticExecutionsNextBlockToCheck);
		EXPECT_EQ(cosignersList, notification.Cosigners);
	}

	// endregion

	// region publish - batch calls notification

	PLUGIN_TEST(CanBatchCallsNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<BatchCallsNotification<1>> sub;
		const auto& config = CreateConfiguration();
		auto pPlugin = TTraits::CreatePlugin(config);
		auto pTransaction = CreateTransaction<TTraits>();
		std::vector<ExtendedCallDigest> callDigests;
		callDigests.reserve(pTransaction->CallsNumber);
		for (uint j = 0; j < pTransaction->CallsNumber; j++) {
			ExtendedCallDigest extendedDigest;
			const auto& shortDigest = pTransaction->CallDigestsPtr()[j];
			extendedDigest.CallId = shortDigest.CallId;
			extendedDigest.Manual = shortDigest.Manual;
			extendedDigest.Status = -1;
			callDigests.push_back(extendedDigest);
		}
		std::vector<model::CallPaymentOpinion> callPaymentOpinions;
		callPaymentOpinions.reserve(pTransaction->CosignersNumber);
		for (uint j = 0; j < pTransaction->CallsNumber; j++) {
			std::vector<Amount> executionWork;
			std::vector<Amount> downloadWork;
			for (uint i = 0; i < pTransaction->CosignersNumber; i++) {
				for (uint j = 0; j < pTransaction->CallsNumber; j++) {
					executionWork.push_back(pTransaction->CallPaymentsPtr()[i * pTransaction->CallsNumber + j].ExecutionPayment);
					downloadWork.push_back(pTransaction->CallPaymentsPtr()[i * pTransaction->CallsNumber + j].DownloadPayment);
				}
				callPaymentOpinions.push_back(CallPaymentOpinion{executionWork, downloadWork});
			}
		}

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];

		EXPECT_EQ(pTransaction->ContractKey, notification.ContractKey);
		EXPECT_EQ(pTransaction->ContractKey, notification.ContractKey);
		for (uint i = 0; i < pTransaction->CallsNumber; i++) {
			// call digest
			EXPECT_EQ(callDigests[i].Block, notification.Digests[i].Block);
			EXPECT_EQ(callDigests[i].CallId, notification.Digests[i].CallId);
			EXPECT_EQ(callDigests[i].Manual, notification.Digests[i].Manual);
			EXPECT_EQ(callDigests[i].ReleasedTransactionHash, notification.Digests[i].ReleasedTransactionHash);
			EXPECT_EQ(callDigests[i].Status, notification.Digests[i].Status);
			// call payment
			for (uint j = 0; j < pTransaction->CallsNumber; j++) {
				EXPECT_EQ(callPaymentOpinions[i].ExecutionWork[j], notification.PaymentOpinions[i].ExecutionWork[j]);
				EXPECT_EQ(callPaymentOpinions[i].DownloadWork[j], notification.PaymentOpinions[i].DownloadWork[j]);
			}
		}
	}

	// endregion
}}

