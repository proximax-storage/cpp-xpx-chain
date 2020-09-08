/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/JoinToDriveTransactionPlugin.h"
#include "src/model/JoinToDriveTransaction.h"
#include "plugins/txes/multisig/src/model/MultisigNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/ServiceTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS JoinToDriveTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(JoinToDrive, std::shared_ptr<config::BlockchainConfigurationHolder>, 1, 1,)

		constexpr UnresolvedMosaicId Storage_Mosaic_Id(1234);
		constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;

		auto CreateConfiguration() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.StorageMosaicId = MosaicId(Storage_Mosaic_Id.unwrap());
			config.Immutable.NetworkIdentifier = Network_Identifier;
			return config.ToConst();
		}

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateJoinToDriveTransaction<typename TTraits::TransactionType>();
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(
		TEST_CLASS,
		,
		,Entity_Type_JoinToDrive,
		config::CreateMockConfigurationHolder(CreateConfiguration()))

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		auto realSize = pPlugin->calculateRealSize(*pTransaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType), realSize);
	}

	// region publish - basic

	PLUGIN_TEST(PublishesNoNotificationWhenTransactionVersionIsInvalid) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));

		typename TTraits::TransactionType transaction;
		transaction.Version = MakeVersion(NetworkIdentifier::Mijin_Test, std::numeric_limits<uint32_t>::max());
		transaction.Size = sizeof(transaction);

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
		ASSERT_EQ(6, sub.numNotifications());
		EXPECT_EQ(Service_Drive_v1_Notification, sub.notificationTypes()[0]);
		EXPECT_EQ(Multisig_Modify_New_Cosigner_v1_Notification, sub.notificationTypes()[1]);
		EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[2]);
		EXPECT_EQ(Multisig_Modify_Cosigners_v1_Notification, sub.notificationTypes()[3]);
		EXPECT_EQ(Service_Join_To_Drive_v1_Notification, sub.notificationTypes()[4]);
		EXPECT_EQ(Core_Balance_Debit_v1_Notification, sub.notificationTypes()[5]);
	}

	// endregion

	// region publish - drive notification

	PLUGIN_TEST(CanPublishDriveNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<DriveNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
		EXPECT_EQ(Entity_Type_JoinToDrive, notification.TransactionType);
	}

	// endregion

	// region publish - modify multisig new cosigner notification

	PLUGIN_TEST(CanPublishModifyMultisigNewCosignerNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ModifyMultisigNewCosignerNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->DriveKey, notification.MultisigAccountKey);
		EXPECT_EQ(pTransaction->Signer, notification.CosignatoryKey);
	}

	// endregion

	// region publish - account public key notification

	PLUGIN_TEST(CanPublishAccountPublicKeyNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<AccountPublicKeyNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->DriveKey, notification.PublicKey);
	}

	// endregion

	// region publish - modify multisig cosigners notification

	PLUGIN_TEST(CanPublishModifyMultisigCosignersNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ModifyMultisigCosignersNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->DriveKey, notification.Signer);
		EXPECT_EQ(1, notification.ModificationsCount);
		auto pModification = notification.ModificationsPtr;
		EXPECT_EQ(CosignatoryModificationType::Add, pModification->ModificationType);
		EXPECT_EQ(pTransaction->Signer, pModification->CosignatoryPublicKey);
	}

	// endregion

	// region publish - join to drive notification

	PLUGIN_TEST(CanPublishJoinToDriveNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<JoinToDriveNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.Replicator);
		EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
	}

	// endregion

	// region publish - balance debit notification

	PLUGIN_TEST(CanPublishBalanceDebitNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<BalanceDebitNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));

		auto pTransaction = CreateTransaction<TTraits>();
		pTransaction->Signer = test::GenerateRandomByteArray<Key>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.Sender);
		EXPECT_EQ(Storage_Mosaic_Id, notification.MosaicId);
		EXPECT_EQ(0u, notification.Amount.unwrap());
		EXPECT_EQ(UnresolvedAmountType::DriveDeposit, notification.Amount.Type);
		auto pDeposit = dynamic_cast<const model::DriveDeposit*>(notification.Amount.DataPtr);
		EXPECT_EQ(pTransaction->DriveKey, pDeposit->DriveKey);
	}

	// endregion
}}
