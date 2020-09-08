/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/utils/HexParser.h"
#include "src/plugins/StartFileDownloadTransactionPlugin.h"
#include "src/model/StartFileDownloadTransaction.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/ServiceTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS StartFileDownloadTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(StartFileDownload, config::ImmutableConfiguration, 1, 1,)

		constexpr UnresolvedMosaicId Streaming_Mosaic_Id(1234);
		constexpr auto Network_Identifier = NetworkIdentifier::Mijin_Test;
		constexpr auto Num_Files = 3u;
		const auto Generation_Hash = utils::ParseByteArray<GenerationHash>("CE076EF4ABFBC65B046987429E274EC31506D173E91BF102F16BEB7FB8176230");

		auto CreateConfiguration() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.StreamingMosaicId = MosaicId(Streaming_Mosaic_Id.unwrap());
			config.Immutable.NetworkIdentifier = Network_Identifier;
			config.Immutable.GenerationHash = Generation_Hash;
			return config.ToConst().Immutable;
		}

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateStartFileDownloadTransaction<typename TTraits::TransactionType>(Num_Files);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(
		TEST_CLASS,
		,
		,Entity_Type_StartFileDownload,
		CreateConfiguration())

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		auto realSize = pPlugin->calculateRealSize(*pTransaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + Num_Files * sizeof(DownloadAction), realSize);
	}

	// region publish - basic

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
		EXPECT_EQ(Service_Drive_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Service_StartFileDownload_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Balance_Debit_v1_Notification, sub.notificationTypes()[i++]);
	}

	// endregion

	// region publish - drive notification

	PLUGIN_TEST(CanPublishDriveNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<DriveNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
		EXPECT_EQ(Entity_Type_StartFileDownload, notification.TransactionType);
	}

	// endregion

	// region publish - start file download notification

	PLUGIN_TEST(CanPublishStartFileDownloadNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<StartFileDownloadNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
		EXPECT_EQ(pTransaction->Signer, notification.FileRecipient);
		EXPECT_EQ(Num_Files, notification.FileCount);
		EXPECT_EQ_MEMORY(pTransaction->FilesPtr(), notification.FilesPtr, Num_Files * sizeof(DownloadAction));
	}

	// endregion

	// region publish - balance debit notification

	PLUGIN_TEST(CanPublishBalanceDebitNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<BalanceDebitNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.Sender);
		EXPECT_EQ(Streaming_Mosaic_Id, notification.MosaicId);
		EXPECT_EQ(Amount(Num_Files * (Num_Files + 1) / 2 * 100), notification.Amount);
	}

	// endregion
}}
