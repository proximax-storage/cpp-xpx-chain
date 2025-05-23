/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/DeactivateTransactionPlugin.h"
#include "src/model/DeactivateTransaction.h"
#include "src/model/SuperContractNotifications.h"
#include "plugins/txes/multisig/src/model/MultisigNotifications.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS DeactivateTransactionPluginTests

	// region TransactionPlugin

	namespace {
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(Deactivate, 1, 1,)

		static const auto Generation_Hash = test::GenerateRandomByteArray<GenerationHash>();

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateDeactivateTransaction<typename TTraits::TransactionType>();
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_Deactivate)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();
		typename TTraits::TransactionType transaction;
		transaction.Size = sizeof(transaction);

		// Act:
		auto realSize = pPlugin->calculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType), realSize);
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
		EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(SuperContract_SuperContract_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(SuperContract_Deactivate_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Multisig_Modify_Cosigners_v1_Notification, sub.notificationTypes()[i++]);
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
		EXPECT_EQ(pTransaction->SuperContract, notification.SuperContract);
	}

	// endregion

	// region deactivate notification

	PLUGIN_TEST(CanPublishDeactivateNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<DeactivateNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.Signer);
		EXPECT_EQ(pTransaction->SuperContract, notification.SuperContract);
		EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
	}

	// endregion

	// region publish modify multisig cosigners notification

	PLUGIN_TEST(CanPublishModifyMultisigCosignersNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ModifyMultisigCosignersNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->SuperContract, notification.Signer);
		EXPECT_EQ(false, notification.AllowMultipleRemove);
		EXPECT_EQ(1, notification.ModificationsCount);
		auto pModification = notification.ModificationsPtr;
		EXPECT_EQ(CosignatoryModificationType::Del, pModification->ModificationType);
		EXPECT_EQ(pTransaction->DriveKey, pModification->CosignatoryPublicKey);
	}

	// endregion
}}
