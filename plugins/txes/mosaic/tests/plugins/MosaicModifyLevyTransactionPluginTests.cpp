/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#include "src/plugins/MosaicModifyLevyTransactionPlugin.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "src/model/MosaicNotifications.h"
#include "src/model/MosaicModifyLevyTransaction.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/TestHarness.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS MosaicModifyLevyTransactionPluginTests

	// region TransactionPlugin

	namespace {
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(MosaicModifyLevy, 1, 1,)

		constexpr auto Transaction_Version = MakeVersion(model::NetworkIdentifier::Mijin_Test, 1);
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , Entity_Type_Mosaic_Modify_Levy)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin();

		typename TTraits::TransactionType transaction;
		transaction.Version = Transaction_Version;
		transaction.Size = 0;

		// Act:
		auto realSize = pPlugin->calculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType), realSize);
	}

	PLUGIN_TEST(CanPublishCorrectNumberOfNotifications) {
		// Arrange:
		mocks::MockNotificationSubscriber sub;
		auto pPlugin = TTraits::CreatePlugin();

		typename TTraits::TransactionType transaction;
		transaction.Size = sizeof(transaction);
		transaction.Version = Transaction_Version;
		
		// Act:
		test::PublishTransaction(*pPlugin, transaction, sub);

		// Assert:
		EXPECT_EQ(1u, sub.numNotifications());
	}

	PLUGIN_TEST(CanPublishMosaicModifyLevyNotifiation) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<MosaicModifyLevyNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin();

		typename TTraits::TransactionType transaction;
		transaction.Size = sizeof(transaction);
		transaction.Version = Transaction_Version;
		test::FillWithRandomData(transaction.Signer);
		transaction.MosaicId = UnresolvedMosaicId(123);
		transaction.Levy = MosaicLevyRaw(LevyType::None, catapult::UnresolvedAddress(), UnresolvedMosaicId(0),	Amount(0));

		// Act:
		test::PublishTransaction(*pPlugin, transaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(transaction.Signer, notification.Signer);
		EXPECT_EQ(transaction.MosaicId, notification.MosaicId);
		EXPECT_EQ(transaction.Levy.Type, notification.Levy.Type);
		EXPECT_EQ(transaction.Levy.Recipient, notification.Levy.Recipient);
		EXPECT_EQ(transaction.Levy.MosaicId, notification.Levy.MosaicId);
		EXPECT_EQ(transaction.Levy.Fee, notification.Levy.Fee);
	}

	// endregion
}}
