/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "sdk/src/extensions/ConversionExtensions.h"
#include <catapult/model/Address.h>
#include "catapult/utils/HexParser.h"
#include "src/plugins/StreamFinishTransactionPlugin.h"
#include "src/model/StreamFinishTransaction.h"
#include "src/catapult/model/StorageNotifications.h"
#include "catapult/model/StreamingNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/StreamingTestUtils.h"
#include "tests/test/nodeps/TestConstants.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS StreamFinishTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(StreamFinish, config::ImmutableConfiguration, 1, 1,)

		const auto Generation_Hash = utils::ParseByteArray<GenerationHash>("CE076EF4ABFBC65B046987429E274EC31506D173E91BF102F16BEB7FB8176230");
		constexpr auto Network_Identifier = NetworkIdentifier::Mijin_Test;

		auto CreateConfiguration() {
			auto config = config::ImmutableConfiguration::Uninitialized();
			config.GenerationHash = Generation_Hash;
			config.NetworkIdentifier = Network_Identifier;
			config.CurrencyMosaicId = test::Default_Currency_Mosaic_Id;
			config.StreamingMosaicId = test::Default_Streaming_Mosaic_Id;
			return config;
		}

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateStreamFinishTransaction<typename TTraits::TransactionType>();
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , Entity_Type_StreamFinish, CreateConfiguration())

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
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
		ASSERT_EQ(2u, sub.numNotifications());
		auto i = 0u;
		EXPECT_EQ(Storage_Drive_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Storage_Stream_Finish_v1_Notification, sub.notificationTypes()[i++]);
	}

	// endregion

	// region publish - drive notification

	PLUGIN_TEST(CanPublishSDriveNotification) {
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
		EXPECT_EQ(Entity_Type_StreamFinish, notification.TransactionType);
	}

	// endregion

	// region publish - stream finish notification

	PLUGIN_TEST(CanPublishStreamFinishNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<StreamFinishNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->DriveKey, notification.DriveKey);
		EXPECT_EQ(pTransaction->StreamId, notification.StreamId);
		EXPECT_EQ(pTransaction->ActualUploadSizeMegabytes, notification.ActualUploadSize);
		EXPECT_EQ(pTransaction->StreamStructureCdi, notification.StreamStructureCdi);
	}

	// endregion
}}