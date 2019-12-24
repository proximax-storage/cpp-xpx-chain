/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "catapult/utils/MemoryUtils.h"
#include "src/plugins/EndDriveVerificationTransactionPlugin.h"
#include "src/model/EndDriveVerificationTransaction.h"
#include "src/model/ServiceNotifications.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "plugins/txes/multisig/src/model/MultisigNotifications.h"
#include "plugins/txes/lock_secret/src/model/LockHashAlgorithm.h"
#include "plugins/txes/lock_secret/src/model/SecretLockNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/TestHarness.h"
#include <limits>

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS EndDriveVerificationTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(EndDriveVerification, NetworkIdentifier, 1, 1,)

		constexpr auto Network_Identifier = NetworkIdentifier::Mijin_Test;
		constexpr auto Num_Failures = 3u;
		constexpr auto Num_Block_Hashes = 5u;

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateEndDriveVerificationTransaction<typename TTraits::TransactionType>(Num_Failures);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_End_Drive_Verification, Network_Identifier)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(Network_Identifier);
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		auto realSize = pPlugin->calculateRealSize(*pTransaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) +
			Num_Failures * (sizeof(VerificationFailure) + Num_Block_Hashes * sizeof(Hash256)), realSize);
	}

	// region publish - basic

	PLUGIN_TEST(PublishesNoNotificationWhenTransactionVersionIsInvalid) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin(Network_Identifier);

		typename TTraits::TransactionType transaction;
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
		auto pPlugin = TTraits::CreatePlugin(Network_Identifier);

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(5 + Num_Failures, sub.numNotifications());
		auto i = 0u;
		EXPECT_EQ(Service_Drive_v1_Notification, sub.notificationTypes()[i++]);
		for (; i < Num_Failures + 1; ++i)
			EXPECT_EQ(Service_Failed_Block_Hashes_v1_Notification, sub.notificationTypes()[i]);
		EXPECT_EQ(Service_End_Drive_Verification_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(LockSecret_Proof_Publication_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Multisig_Modify_Cosigners_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Service_Drive_Verification_Payment_v1_Notification, sub.notificationTypes()[i++]);
	}

	// endregion

	// region publish - drive notification

	PLUGIN_TEST(CanPublishDriveNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<DriveNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(Network_Identifier);
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.DriveKey);
		EXPECT_EQ(Entity_Type_End_Drive_Verification, notification.TransactionType);
	}

	// endregion

	// region publish - end drive verification notification

	PLUGIN_TEST(CanPublishFailedBlockHashesNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<FailedBlockHashesNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(Network_Identifier);
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(Num_Failures, sub.numMatchingNotifications());
		auto failures = pTransaction->Transactions();
		auto i = 0u;
		for (auto iter = failures.begin(); iter != failures.end(); ++iter) {
			const auto& notification = sub.matchingNotifications()[i++];
			auto blockHashCount = iter->BlockHashCount();
			EXPECT_EQ(blockHashCount, notification.BlockHashCount);
			EXPECT_EQ_MEMORY(iter->BlockHashesPtr(), notification.BlockHashesPtr, blockHashCount * Hash256_Size);
		}
	}

	// endregion

	// region publish - end drive verification notification

	PLUGIN_TEST(CanPublishEndDriveVerificationNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<EndDriveVerificationNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(Network_Identifier);
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.DriveKey);
		EXPECT_EQ(Num_Failures, notification.FailureCount);
		auto failures = pTransaction->Transactions();
		auto i = 0u;
		for (auto iter = failures.begin(); iter != failures.end(); ++iter)
			EXPECT_EQ(iter->Replicator, notification.FailedReplicatorsPtr[i++]);
	}

	// endregion

	// region publish - proof publication notification

	PLUGIN_TEST(CanPublishProofPublicationNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ProofPublicationNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(Network_Identifier);
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.Signer);
		EXPECT_EQ(LockHashAlgorithm::Op_Internal, notification.HashAlgorithm);
		EXPECT_EQ(Hash256(), notification.Secret);
		auto recipient = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(pTransaction->Signer, Network_Identifier));
		EXPECT_EQ(recipient, notification.Recipient);
	}

	// endregion

	// region publish - modify multisig cosigners notification

	PLUGIN_TEST(CanPublishModifyMultisigCosignersNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ModifyMultisigCosignersNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(Network_Identifier);
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.Signer);
		EXPECT_EQ(Num_Failures, notification.ModificationsCount);
		EXPECT_TRUE(notification.AllowMultipleRemove);
		auto failures = pTransaction->Transactions();
		auto pModifications = notification.ModificationsPtr;
		for (auto iter = failures.begin(); iter != failures.end(); ++iter, ++pModifications) {
			EXPECT_EQ(CosignatoryModificationType::Del, pModifications->ModificationType);
			EXPECT_EQ(iter->Replicator, pModifications->CosignatoryPublicKey);
		}
	}

	// endregion

	// region publish - drive verification payment notification

	PLUGIN_TEST(CanPublishDriveVerificationPaymentNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<DriveVerificationPaymentNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(Network_Identifier);
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.DriveKey);
		EXPECT_EQ(Num_Failures, notification.FailureCount);
		auto failures = pTransaction->Transactions();
		auto i = 0u;
		for (auto iter = failures.begin(); iter != failures.end(); ++iter)
			EXPECT_EQ(iter->Replicator, notification.FailedReplicatorsPtr[i++]);
	}

	// endregion
}}
