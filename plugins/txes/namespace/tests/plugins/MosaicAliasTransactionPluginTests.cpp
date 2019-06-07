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

#include "src/plugins/MosaicAliasTransactionPlugin.h"
#include "src/model/MosaicAliasTransaction.h"
#include "tests/test/AliasTransactionPluginTests.h"
#include "tests/test/core/mocks/MockSupportedVersionSupplier.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS MosaicAliasTransactionPluginTests

	// region TransactionPlugin

	namespace {
		DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(MosaicAlias)

		mocks::MockSupportedVersionSupplier Supported_Versions_Supplier({ 1 });

		struct NotificationTraits {
		public:
			using Notification_Type = model::AliasedMosaicIdNotification_v1;

		public:
			static constexpr size_t NumNotifications() {
				return 3u;
			}

		public:
			template<typename TTransaction>
			static auto& TransactionAlias(TTransaction& transaction) {
				return transaction.MosaicId;
			}
		};
	}

	DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, Entity_Type_Alias_Mosaic, Supported_Versions_Supplier)

	DEFINE_ALIAS_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, MosaicAlias, NotificationTraits, Supported_Versions_Supplier)

	PLUGIN_TEST(CanExtractMosaicRequiredNotification) {
		// Arrange:
		mocks::MockTypedNotificationSubscriber<model::MosaicRequiredNotification<1>> mosaicSub;
		auto pPlugin = TTraits::CreatePlugin(Supported_Versions_Supplier);

		typename TTraits::TransactionType transaction;
		transaction.Version = MakeVersion(model::NetworkIdentifier::Mijin_Test, 1);
		transaction.NamespaceId = NamespaceId(123);
		transaction.AliasAction = model::AliasAction::Unlink;
		transaction.Signer = test::GenerateRandomData<Key_Size>();
		transaction.MosaicId = test::GenerateRandomValue<MosaicId>();

		// Act:
		test::PublishTransaction(*pPlugin, transaction, mosaicSub);

		// Assert:
		ASSERT_EQ(1u, mosaicSub.numMatchingNotifications());
		const auto& notification = mosaicSub.matchingNotifications()[0];
		EXPECT_EQ(transaction.Signer, notification.Signer);
		EXPECT_EQ(transaction.MosaicId, notification.MosaicId);
		EXPECT_EQ(UnresolvedMosaicId(), notification.UnresolvedMosaicId);
		EXPECT_EQ(MosaicRequiredNotification<1>::MosaicType::Resolved, notification.ProvidedMosaicType);
	}

	// endregion
}}
