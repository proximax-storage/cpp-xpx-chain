/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include <tests/test/other/MutableBlockchainConfiguration.h>
#include <tests/test/local/LocalNodeTestState.h>
#include "src/plugins/MosaicSupplyChangeTransactionPlugin.h"
#include "src/model/MosaicNotifications.h"
#include "src/model/MosaicSupplyChangeTransaction.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS MosaicSupplyChangeTransactionPluginTests

	// region TransactionPlugin

	namespace {
		DEFINE_TRANSACTION_PLUGIN_WITH_CONFIG_TEST_TRAITS(MosaicSupplyChange, std::shared_ptr<config::BlockchainConfigurationHolder>, 2, 2,)

		constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;

		auto CreateConfiguration() {
			test::MutableBlockchainConfiguration config;
			return config.ToConst();
		}

		constexpr auto Transaction_Version = MakeVersion(model::NetworkIdentifier::Mijin_Test, 2);
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , Entity_Type_Mosaic_Supply_Change,
													 config::CreateMockConfigurationHolder(CreateConfiguration()))

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));

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
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));

		typename TTraits::TransactionType transaction;
		transaction.Size = sizeof(transaction);
		transaction.Version = Transaction_Version;

		// Act:
		test::PublishTransaction(*pPlugin, transaction, sub);

		// Assert:
		EXPECT_EQ(2u, sub.numNotifications());
	}

	PLUGIN_TEST(CanPublishMosaicRequiredNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<MosaicRequiredNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));

		typename TTraits::TransactionType transaction;
		transaction.Size = sizeof(transaction);
		transaction.Version = Transaction_Version;
		test::FillWithRandomData(transaction.Signer);
		transaction.MosaicId = test::GenerateRandomValue<UnresolvedMosaicId>();

		// Act:
		test::PublishTransaction(*pPlugin, transaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(transaction.Signer, notification.Signer);
		EXPECT_EQ(MosaicId(), notification.MosaicId);
		EXPECT_EQ(transaction.MosaicId, notification.UnresolvedMosaicId);
		EXPECT_EQ(MosaicRequiredNotification<1>::MosaicType::Unresolved, notification.ProvidedMosaicType);
	}

	PLUGIN_TEST(CanPublishMosaicSupplyChangeNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<MosaicSupplyChangeNotification<1>> sub;
		auto pPlugin = TTraits::CreatePlugin(config::CreateMockConfigurationHolder(CreateConfiguration()));

		typename TTraits::TransactionType transaction;
		transaction.Size = sizeof(transaction);
		transaction.Version = Transaction_Version;
		test::FillWithRandomData(transaction.Signer);
		transaction.MosaicId = test::GenerateRandomValue<UnresolvedMosaicId>();
		transaction.Direction = MosaicSupplyChangeDirection::Increase;
		transaction.Delta = Amount(787);

		// Act:
		test::PublishTransaction(*pPlugin, transaction, sub);

		// Assert:
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		const auto& notification = sub.matchingNotifications()[0];
		EXPECT_EQ(transaction.Signer, notification.Signer);
		EXPECT_EQ(transaction.MosaicId, notification.MosaicId);
		EXPECT_EQ(MosaicSupplyChangeDirection::Increase, notification.Direction);
		EXPECT_EQ(Amount(787), notification.Delta);
	}

	// endregion
}}
