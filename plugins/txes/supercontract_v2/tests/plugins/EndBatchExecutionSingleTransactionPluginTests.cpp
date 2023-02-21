/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/EndBatchExecutionSingleTransactionPlugin.h"
#include "src/model/EndBatchExecutionSingleTransaction.h"
#include "src/model/InternalSuperContractNotifications.h"
#include "src/catapult/model/SupercontractNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/SuperContractTestUtils.h"
#include <tests/test/nodeps/TestConstants.h>
#include "catapult/utils/HexParser.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS EndBatchExecutionSingleTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(EndBatchExecutionSingle, config::ImmutableConfiguration, 1, 1,)

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
			return test::CreateEndBatchExecutionSingleTransaction<typename TTraits::TransactionType>();
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_EndBatchExecutionSingleTransaction, CreateConfiguration())

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		auto realSize = pPlugin->calculateRealSize(*pTransaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType), realSize);
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
		ASSERT_EQ(3u, sub.numNotifications());
		auto i = 0u;
		EXPECT_EQ(SuperContract_v2_Contract_State_Update_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(SuperContract_v2_Unsuccessful_Batch_Execution_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(SuperContract_v2_Proof_Of_Execution_v1_Notification, sub.notificationTypes()[i++]);
	}

	// endregion

	// region publish - end batch execution single notification

	PLUGIN_TEST(CanPublishEndBatchExecutionSingleNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<BatchExecutionSingleNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->ContractKey, notification.ContractKey);
		EXPECT_EQ(pTransaction->BatchId, notification.BatchId);
		EXPECT_EQ(pTransaction->Signer, notification.Signer);
	}

	// endregion

	// region publish - contract state update notification

	PLUGIN_TEST(CanPublishContractStateUpdateNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ContractStateUpdateNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->ContractKey, notification.ContractKey);
	}

	// endregion

	// region publish - proof of execution notification

	PLUGIN_TEST(CanPublishProofOfExecutionNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ProofOfExecutionNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();
		std::map<Key, ProofOfExecution> proofs;
		const auto& rawProof = pTransaction->ProofOfExecution;
		model::ProofOfExecution proof;
		proof.StartBatchId = rawProof.StartBatchId;

		proof.T.fromBytes(rawProof.T);
		proof.R = crypto::Scalar(rawProof.R);
		proof.F.fromBytes(rawProof.F);
		proof.K = crypto::Scalar(rawProof.K);
		proofs[pTransaction->Signer] = proof;

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);
		auto pred = [] (auto a, auto b) {
			return a.first == b.first &&
			a.second.StartBatchId == b.second.StartBatchId &&
			a.second.T == b.second.T &&
			a.second.R == b.second.R &&
			a.second.F == b.second.F &&
			a.second.K == b.second.K;
		};

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->ContractKey, notification.ContractKey);
		EXPECT_TRUE(std::equal(proofs.begin(), proofs.end(), notification.Proofs.begin(), pred));
	}

	// endregion
}}
