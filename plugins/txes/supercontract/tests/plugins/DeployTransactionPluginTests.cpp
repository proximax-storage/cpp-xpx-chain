/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/DeployTransactionPlugin.h"
#include "src/model/DeployTransaction.h"
#include "src/model/SuperContractNotifications.h"
#include "plugins/txes/multisig/src/model/MultisigNotifications.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS DeployTransactionPluginTests

	// region TransactionPlugin

	namespace {
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(Deploy, 1, 1,)

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateDeployTransaction<typename TTraits::TransactionType>();
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS_ONLY_EMBEDDABLE(TEST_CLASS,,, Entity_Type_Deploy)

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
		ASSERT_EQ(7u, sub.numNotifications());
		EXPECT_EQ(Service_Drive_v1_Notification, sub.notificationTypes()[0]);
		EXPECT_EQ(Multisig_Modify_New_Cosigner_v1_Notification, sub.notificationTypes()[1]);
		EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[2]);
		EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[3]);
		EXPECT_EQ(Multisig_Modify_Cosigners_v1_Notification, sub.notificationTypes()[4]);
		EXPECT_EQ(Multisig_Modify_Settings_v1_Notification, sub.notificationTypes()[5]);
		EXPECT_EQ(SuperContract_Deploy_v1_Notification, sub.notificationTypes()[6]);
	}

	// endregion

	PLUGIN_TEST(CanPublishDriveNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<DriveNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
		EXPECT_EQ(pTransaction->Type, notification.TransactionType);
	}

	PLUGIN_TEST(CanPublishModifyMultisigNewCosignerNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ModifyMultisigNewCosignerNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.MultisigAccountKey);
		EXPECT_EQ(pTransaction->DriveKey, notification.CosignatoryKey);
	}

	PLUGIN_TEST(CanPublishAccountPublicKeyNotifications) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<AccountPublicKeyNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(2u, sub.numMatchingNotifications());
		EXPECT_EQ(pTransaction->Owner, sub.matchingNotifications()[0].PublicKey);
		EXPECT_EQ(pTransaction->DriveKey, sub.matchingNotifications()[1].PublicKey);
	}

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
		EXPECT_EQ(pTransaction->Signer, notification.Signer);
		EXPECT_EQ(1, notification.ModificationsCount);
		EXPECT_EQ(model::CosignatoryModificationType::Add, notification.ModificationsPtr->ModificationType);
		EXPECT_EQ(pTransaction->DriveKey, notification.ModificationsPtr->CosignatoryPublicKey);
		EXPECT_EQ(false, notification.AllowMultipleRemove);
	}

	PLUGIN_TEST(CanPublishModifyMultisigSettingsNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ModifyMultisigSettingsNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.Signer);
		EXPECT_EQ(1, notification.MinRemovalDelta);
		EXPECT_EQ(1, notification.MinApprovalDelta);
	}

	PLUGIN_TEST(CanPublishDeployNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<DeployNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];

		EXPECT_EQ(pTransaction->Signer, notification.SuperContract);
		EXPECT_EQ(pTransaction->Owner, notification.Owner);
		EXPECT_EQ(pTransaction->DriveKey, notification.Drive);
		EXPECT_EQ(pTransaction->FileHash, notification.FileHash);
		EXPECT_EQ(pTransaction->VmVersion, notification.VmVersion);
	}
}}
