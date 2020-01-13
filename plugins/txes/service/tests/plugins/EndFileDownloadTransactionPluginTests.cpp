/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "plugins/txes/lock_secret/src/model/LockHashAlgorithm.h"
#include "plugins/txes/lock_secret/src/model/SecretLockNotifications.h"
#include "src/plugins/EndFileDownloadTransactionPlugin.h"
#include "src/model/EndFileDownloadTransaction.h"
#include "src/model/ServiceNotifications.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/test/ServiceTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS EndFileDownloadTransactionPluginTests

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(EndFileDownload, config::ImmutableConfiguration, 1, 1,)

		constexpr UnresolvedMosaicId Review_Mosaic_Id(1234);
		constexpr auto Network_Identifier = NetworkIdentifier::Mijin_Test;
		constexpr auto Num_Files = 3u;

		auto CreateConfiguration() {
			auto config = config::ImmutableConfiguration::Uninitialized();
			config.ReviewMosaicId = MosaicId(Review_Mosaic_Id.unwrap());
			config.NetworkIdentifier = Network_Identifier;
			return config;
		}

		template<typename TTraits>
		auto CreateTransaction() {
			return test::CreateEndFileDownloadTransaction<typename TTraits::TransactionType>(Num_Files);
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(
		TEST_CLASS,
		,
		,Entity_Type_EndFileDownload,
		CreateConfiguration())

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
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
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());

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
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(3u + Num_Files, sub.numNotifications());
		auto i = 0u;
		EXPECT_EQ(Service_Drive_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Register_Account_Public_Key_v1_Notification, sub.notificationTypes()[i++]);
		EXPECT_EQ(Core_Balance_Credit_v1_Notification, sub.notificationTypes()[i++]);
		while (i < 3u + Num_Files) {
			EXPECT_EQ(LockSecret_Proof_Publication_v1_Notification, sub.notificationTypes()[i++]);
		}
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
		EXPECT_EQ(pTransaction->Signer, notification.DriveKey);
		EXPECT_EQ(Entity_Type_EndFileDownload, notification.TransactionType);
	}

	// endregion

	// region publish - account public key notification

	PLUGIN_TEST(CanPublishAccountPublicKeyNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<AccountPublicKeyNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Recipient, notification.PublicKey);
	}

	// endregion

	// region publish - balance credit notification

	PLUGIN_TEST(CanPublishBalanceCreditNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<BalanceCreditNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Recipient, notification.Sender);
		EXPECT_EQ(Review_Mosaic_Id, notification.MosaicId);
		EXPECT_EQ(Amount(Num_Files), notification.Amount);
	}

	// endregion

	// region publish - proof publication notifications

	PLUGIN_TEST(CanPublishProofPublicationNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<ProofPublicationNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(CreateConfiguration());
		auto pTransaction = CreateTransaction<TTraits>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(Num_Files, sub.numMatchingNotifications());
		for (auto i = 0u; i < Num_Files; ++i) {
			const auto& notification = sub.matchingNotifications()[i];
			EXPECT_EQ(pTransaction->Signer, notification.Signer);
			EXPECT_EQ(LockHashAlgorithm::Op_Internal, notification.HashAlgorithm);
			auto secret = Hash256(pTransaction->Recipient.array()) ^ Hash256(pTransaction->Signer.array()) ^ pTransaction->FilesPtr()[i].FileHash;
			EXPECT_EQ(secret, notification.Secret);
			auto recipient = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(pTransaction->Signer, Network_Identifier));
			EXPECT_EQ(recipient, notification.Recipient);
		}
	}

	// endregion
}}
