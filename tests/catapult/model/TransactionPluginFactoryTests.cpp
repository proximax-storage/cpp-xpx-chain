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

#include "catapult/model/TransactionPluginFactory.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/core/mocks/MockSupportedVersionSupplier.h"
#include "tests/test/core/mocks/MockTransaction.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

#define TEST_CLASS TransactionPluginFactoryTests

	namespace {
		constexpr auto Mock_Transaction_Type = static_cast<EntityType>(0x4FFF);
		mocks::MockSupportedVersionSupplier Supported_Versions_Supplier({ 1 });

		template<typename TTransaction>
		void Publish(const TTransaction& transaction, NotificationSubscriber& sub) {
			// raise a notification dependent on the transaction data
			sub.notify(test::CreateBlockNotification(transaction.Signer));
		}

		struct RegularTraits {
			using TransactionType = mocks::MockTransaction;
			static VersionSet supportedVersions;

			static auto CreatePlugin(SupportedVersionsSupplier supportedVersionsSupplier) {
				supportedVersions = supportedVersionsSupplier();
				return TransactionPluginFactory::Create<mocks::MockTransaction, mocks::EmbeddedMockTransaction>(
					Publish<mocks::MockTransaction>,
					Publish<mocks::EmbeddedMockTransaction>,
					supportedVersionsSupplier);
			}
		};
		VersionSet RegularTraits::supportedVersions = VersionSet{};

		struct EmbeddedTraits {
			using TransactionType = mocks::EmbeddedMockTransaction;
			static VersionSet supportedVersions;

			static auto CreatePlugin(SupportedVersionsSupplier supportedVersionsSupplier) {
				supportedVersions = supportedVersionsSupplier();
				return TransactionPluginFactory::CreateEmbedded<mocks::EmbeddedMockTransaction>(
					Publish<mocks::EmbeddedMockTransaction>,
					supportedVersionsSupplier);
			}
		};
		VersionSet EmbeddedTraits::supportedVersions = VersionSet{};
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS_WITH_PREFIXED_TRAITS(TEST_CLASS, , , Mock_Transaction_Type, Supported_Versions_Supplier)

	PLUGIN_TEST(CanCalculateSize) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(Supported_Versions_Supplier);

		typename TTraits::TransactionType transaction;
		transaction.Size = 0;
		transaction.Data.Size = 100;

		// Act:
		auto realSize = pPlugin->calculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(typename TTraits::TransactionType) + 100, realSize);
	}

	PLUGIN_TEST(CanPublishNotifications) {
		// Arrange:
		auto pPlugin = TTraits::CreatePlugin(Supported_Versions_Supplier);

		typename TTraits::TransactionType transaction;
		test::FillWithRandomData(transaction.Signer);
		mocks::MockTypedNotificationSubscriber<BlockNotification<1>> sub;

		// Act:
		pPlugin->publish(transaction, sub);

		// Assert:
		EXPECT_EQ(1u, sub.numNotifications());
		ASSERT_EQ(1u, sub.numMatchingNotifications());
		EXPECT_EQ(transaction.Signer, sub.matchingNotifications()[0].Signer);
	}
}}
