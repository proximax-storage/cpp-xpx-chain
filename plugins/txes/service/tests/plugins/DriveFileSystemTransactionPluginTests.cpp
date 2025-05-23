/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "src/plugins/DriveFileSystemTransactionPlugin.h"
#include "src/model/DriveFileSystemTransaction.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/ServiceTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS DriveFileSystemTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(DriveFileSystem, std::shared_ptr<config::BlockchainConfigurationHolder>, 1, 1,)

		constexpr UnresolvedMosaicId Streaming_Mosaic_Id(1234);
		constexpr auto Network_Identifier = NetworkIdentifier::Mijin_Test;
		constexpr auto Num_Add_Actions = 3u;
		constexpr auto Num_Remove_Actions = 4u;

		auto CreateConfiguration() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.StreamingMosaicId = MosaicId(Streaming_Mosaic_Id.unwrap());
			config.Immutable.NetworkIdentifier = Network_Identifier;
			return config.ToConst();
		}

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateDriveFileSystemTransaction<typename TTraits::TransactionType>(Num_Add_Actions, Num_Remove_Actions);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(
		TEST_CLASS,
		,
		,Entity_Type_DriveFileSystem,
		config::CreateMockConfigurationHolder(CreateConfiguration()))

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		auto realSize = pPlugin->calculateRealSize(*pTransaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) +
			Num_Add_Actions * sizeof(AddAction) + Num_Remove_Actions * sizeof(RemoveAction), realSize);
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
		ASSERT_EQ(3u + Num_Add_Actions, sub.numNotifications());
		EXPECT_EQ(Service_Drive_v1_Notification, sub.notificationTypes()[0]);
		EXPECT_EQ(Service_Drive_File_System_v1_Notification, sub.notificationTypes()[1]);
		EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[2]);
		for (auto i = 0u; i < Num_Add_Actions; ++i)
			EXPECT_EQ(Core_Balance_Transfer_v1_Notification, sub.notificationTypes()[i + 3u]);
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
		EXPECT_EQ(Entity_Type_DriveFileSystem, notification.TransactionType);
	}

	// endregion

	// region publish - drive file system notification

	PLUGIN_TEST(CanPublishDriveFileSystemNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<DriveFileSystemNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.Signer);
		EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
		EXPECT_EQ(pTransaction->RootHash, notification.RootHash);
		EXPECT_EQ(pTransaction->XorRootHash, notification.XorRootHash);
		EXPECT_EQ(pTransaction->AddActionsCount, notification.AddActionsCount);
		EXPECT_EQ_MEMORY(pTransaction->AddActionsPtr(), notification.AddActionsPtr, Num_Add_Actions * sizeof(AddAction));
        EXPECT_EQ(pTransaction->RemoveActionsCount, notification.RemoveActionsCount);
		EXPECT_EQ_MEMORY(pTransaction->RemoveActionsPtr(), notification.RemoveActionsPtr, Num_Remove_Actions * sizeof(RemoveAction));
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

	// region publish - balance transfer notification

	PLUGIN_TEST(CanPublishBalanceTransferNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<BalanceTransferNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));

		auto pTransaction = CreateTransaction<TTraits>();
		pTransaction->Signer = test::GenerateRandomByteArray<Key>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(Num_Add_Actions, sub.numMatchingNotifications());
		auto driveAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(pTransaction->DriveKey, Network_Identifier));
		auto pAddActions = pTransaction->AddActionsPtr();
		for (auto i = 0u; i < Num_Add_Actions; ++i, ++pAddActions) {
			const auto& notification = sub.matchingNotifications()[i];
			EXPECT_EQ(pTransaction->Signer, notification.Sender);
			EXPECT_EQ(driveAddress, notification.Recipient);
			EXPECT_EQ(Streaming_Mosaic_Id, notification.MosaicId);
			EXPECT_EQ(0u, notification.Amount.unwrap());
			EXPECT_EQ(UnresolvedAmountType::FileUpload, notification.Amount.Type);
			auto pDeposit = dynamic_cast<const struct FileUpload*>(notification.Amount.DataPtr);
			EXPECT_EQ(pTransaction->DriveKey, pDeposit->DriveKey);
			EXPECT_EQ(pAddActions->FileSize, pDeposit->FileSize);
		}
	}

	// endregion
}}
