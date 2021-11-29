/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/utils/MemoryUtils.h"
#include "src/plugins/AccountV2UpgradeTransactionPlugin.h"
#include "src/model/AccountV2UpgradeTransaction.h"
#include "src/model/BlockchainUpgradeNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/TestHarness.h"
#include <limits>

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS AccountV2UpgradeTransactionPluginTests

	namespace {
		constexpr auto Transaction_Version = MakeVersion(NetworkIdentifier::Mijin_Test, 1);

		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(AccountV2Upgrade, 1, 1,)

		template<typename TTraits>
		auto CreateTransaction() {
			using TransactionType = typename TTraits::TransactionType;
			uint32_t entitySize = sizeof(TransactionType);
			auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
			pTransaction->Version = Transaction_Version;
			pTransaction->Size = entitySize;
			test::FillWithRandomData(pTransaction->Signer);
			pTransaction->Type = static_cast<model::EntityType>(0x4158);
			return pTransaction;
		}
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_AccountV2_Upgrade)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();

		typename TTraits::TransactionType transaction;

		// Act:
		auto realSize = pPlugin->calculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType), realSize);
	}

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
		ASSERT_EQ(1, sub.numNotifications());
		EXPECT_EQ(BlockchainUpgrade_AccountUpgradeV2_v1_Notification, sub.notificationTypes()[0]);
	}

	// endregion

	// region publish - catapult upgrade signer notification

	PLUGIN_TEST(CanPublishAccountUpgradeV2Notification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<AccountV2UpgradeNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();

		auto pTransaction = CreateTransaction<TTraits>();
		pTransaction->NewAccountPublicKey = test::GenerateRandomByteArray<Key>();
		pTransaction->Signer = test::GenerateRandomByteArray<Key>();
		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->NewAccountPublicKey, notification.NewAccountPublicKey);
		EXPECT_EQ(pTransaction->Signer, notification.Signer);
	}

	// endregion
}}
