/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/utils/MemoryUtils.h"
#include "src/plugins/CatapultUpgradeTransactionPlugin.h"
#include "src/model/CatapultUpgradeTransaction.h"
#include "src/model/CatapultUpgradeNotifications.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/TestHarness.h"
#include <limits>

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS CatapultUpgradeTransactionPluginTests

	namespace {
		constexpr auto Transaction_Version = MakeVersion(NetworkIdentifier::Mijin_Test, 1);

		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(CatapultUpgrade, 1, 1,)

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

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, Entity_Type_Catapult_Upgrade)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();

		typename TTraits::TransactionType transaction;

		// Act:
		auto realSize = pPlugin->calculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType), realSize);
	}

	PLUGIN_TEST(ThrowsWhenTransactionVersionIsInvalid) {
		// Arrange:
		typename TTraits::TransactionType transaction;
		transaction.Version = MakeVersion(NetworkIdentifier::Mijin_Test, std::numeric_limits<uint32_t>::max());
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin();

		// Act + Assert:
		EXPECT_THROW(test::PublishTransaction(*pPlugin, transaction, sub), catapult_runtime_error);
	}

	// region publish - basic

	PLUGIN_TEST(CanPublishCorrectNumberOfNotifications) {
		// Arrange:
		auto pTransaction = CreateTransaction<TTraits>();
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(2, sub.numNotifications());
		EXPECT_EQ(CatapultUpgrade_Signer_v1_Notification, sub.notificationTypes()[0]);
		EXPECT_EQ(CatapultUpgrade_Version_v1_Notification, sub.notificationTypes()[1]);
	}

	// endregion

	// region publish - catapult upgrade signer notification

	PLUGIN_TEST(CanPublishCatapultUpgradeSignerNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<CatapultUpgradeSignerNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();

		auto pTransaction = CreateTransaction<TTraits>();
		pTransaction->Signer = test::GenerateRandomByteArray<Key>();

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(pTransaction->Signer, notification.Signer);
	}

	// endregion

	// region publish - catapult upgrade version notification

	PLUGIN_TEST(CanPublishCatapultUpgradeVersionNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<CatapultUpgradeVersionNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();

		auto pTransaction = CreateTransaction<TTraits>();
		pTransaction->UpgradePeriod = BlockDuration{100};
		pTransaction->NewCatapultVersion = CatapultVersion{7};

		// Act:
		test::PublishTransaction(*pPlugin, *pTransaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(100, notification.UpgradePeriod.unwrap());
		EXPECT_EQ(7, notification.Version.unwrap());
	}

	// endregion
}}
