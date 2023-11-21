/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/plugins/FilesDepositTransactionPlugin.h"
#include "src/model/FilesDepositTransaction.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/ServiceTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS FilesDepositTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(FilesDeposit, std::shared_ptr<config::BlockchainConfigurationHolder>, 1, 1,)

		constexpr UnresolvedMosaicId Streaming_Mosaic_Id(1234);
		constexpr auto Network_Identifier = NetworkIdentifier::Mijin_Test;
		constexpr auto Num_Files = 3u;

		auto CreateConfiguration() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.StreamingMosaicId = MosaicId(Streaming_Mosaic_Id.unwrap());
			config.Immutable.NetworkIdentifier = Network_Identifier;
			return config.ToConst();
		}

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateFilesDepositTransaction<typename TTraits::TransactionType>(Num_Files);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(
		TEST_CLASS,
		,
		,Entity_Type_FilesDeposit,
		config::CreateMockConfigurationHolder(CreateConfiguration()))

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		auto realSize = pPlugin->calculateRealSize(*pTransaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + Num_Files * sizeof(File), realSize);
	}

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
		ASSERT_EQ(3u + Num_Files, sub.numNotifications());
		EXPECT_EQ(Service_Drive_v1_Notification, sub.notificationTypes()[0]);
		EXPECT_EQ(Service_Files_Deposit_v1_Notification, sub.notificationTypes()[1]);
		EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[2]);
		for (auto i = 0u; i < Num_Files; ++i)
			EXPECT_EQ(Core_Balance_Debit_v1_Notification, sub.notificationTypes()[i + 3u]);
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
		EXPECT_EQ(Entity_Type_FilesDeposit, notification.TransactionType);
	}

	// endregion

	// region publish - drive file system notification

	PLUGIN_TEST(CanPublishFilesDepositNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<FilesDepositNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.Replicator);
		EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
		EXPECT_EQ(Num_Files, notification.FilesCount);
		EXPECT_EQ_MEMORY(pTransaction->FilesPtr(), notification.FilesPtr, Num_Files * sizeof(File));
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

	// region publish - balance debit notifications

	PLUGIN_TEST(CanPublishBalanceDebitNotifications) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<BalanceDebitNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));

		auto pTransaction = CreateTransaction<TTraits>();
		pTransaction->Signer = test::GenerateRandomByteArray<Key>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(Num_Files, sub.numMatchingNotifications());
		auto pFile = pTransaction->FilesPtr();
		for (auto i = 0u; i < Num_Files; ++i, ++pFile) {
			const auto& notification = sub.matchingNotifications()[i];
			EXPECT_EQ(pTransaction->Signer, notification.Sender);
			EXPECT_EQ(Streaming_Mosaic_Id, notification.MosaicId);
			EXPECT_EQ(0u, notification.Amount.unwrap());
			EXPECT_EQ(UnresolvedAmountType::FileDeposit, notification.Amount.Type);
			auto pDeposit = dynamic_cast<const struct FileDeposit*>(notification.Amount.DataPtr);
			EXPECT_EQ(pTransaction->DriveKey, pDeposit->DriveKey);
			EXPECT_EQ(pFile->FileHash, pDeposit->FileHash);
		}
	}

	// endregion
}}
