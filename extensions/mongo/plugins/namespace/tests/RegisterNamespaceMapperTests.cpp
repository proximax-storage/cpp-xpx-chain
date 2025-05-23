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

#include "src/RegisterNamespaceMapper.h"
#include "sdk/src/builders/RegisterNamespaceBuilder.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS RegisterNamespaceMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS(RegisterNamespace)

		auto CreateRegisterNamespaceTransactionBuilder(
				const Key& signer,
				model::NamespaceType namespaceType,
				const std::string& namespaceName) {
			builders::RegisterNamespaceBuilder builder(model::NetworkIdentifier::Mijin_Test, signer);
			builder.setName({ reinterpret_cast<const uint8_t*>(namespaceName.data()), namespaceName.size() });

			if (model::NamespaceType::Root == namespaceType)
				builder.setDuration(test::GenerateRandomValue<BlockDuration>());
			else
				builder.setParentId(test::GenerateRandomValue<NamespaceId>());

			return builder;
		}

		void AssertSharedRegisterNamespaceData(
				model::NamespaceType namespaceType,
				NamespaceId id,
				const std::string& namespaceName,
				const bsoncxx::document::view& dbTransaction) {
			// Assert:
			EXPECT_EQ(namespaceType, static_cast<model::NamespaceType>(test::GetUint32(dbTransaction, "namespaceType")));
			EXPECT_EQ(id, NamespaceId(test::GetUint64(dbTransaction, "namespaceId")));

			auto dbName = dbTransaction["name"].get_binary();
			EXPECT_EQ(namespaceName.size(), dbName.size);
			EXPECT_EQ(
					test::ToHexString(reinterpret_cast<const uint8_t*>(namespaceName.data()), namespaceName.size()),
					test::ToHexString(dbName.bytes, dbName.size));
		}
	}

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , model::Entity_Type_Register_Namespace)

	// region streamTransaction

	PLUGIN_TEST(CannotMapRegisterNamespaceTransactionWithoutName) {
		// Arrange:
		typename TTraits::TransactionType transaction;
		transaction.Size = sizeof(typename TTraits::TransactionType);
		transaction.Type = model::Entity_Type_Register_Namespace;
		transaction.NamespaceType = model::NamespaceType::Root;
		transaction.NamespaceNameSize = 0;

		auto pPlugin = TTraits::CreatePlugin();

		// Act + Assert:
		mappers::bson_stream::document builder;
		EXPECT_THROW(pPlugin->streamTransaction(builder, transaction), catapult_runtime_error);
	}

	PLUGIN_TEST(CanMapRootRegisterNamespaceTransactionWithName) {
		// Arrange:
		std::string namespaceName("jabo38");
		auto signer = test::GenerateRandomByteArray<Key>();
		auto pTransaction = TTraits::Adapt(CreateRegisterNamespaceTransactionBuilder(signer, model::NamespaceType::Root, namespaceName));
		auto namespaceId = pTransaction->NamespaceId;
		auto pPlugin = TTraits::CreatePlugin();

		// Act:
		mappers::bson_stream::document builder;
		pPlugin->streamTransaction(builder, *pTransaction);
		auto view = builder.view();

		// Assert:
		EXPECT_EQ(4u, test::GetFieldCount(view));
		AssertSharedRegisterNamespaceData(pTransaction->NamespaceType, namespaceId, namespaceName, view);
		EXPECT_EQ(pTransaction->Duration, BlockDuration(test::GetUint64(view, "duration")));
	}

	PLUGIN_TEST(CanMapChildRegisterNamespaceTransactionWithName) {
		// Arrange:
		std::string namespaceName("jabo38");
		auto signer = test::GenerateRandomByteArray<Key>();
		auto pTransaction = TTraits::Adapt(CreateRegisterNamespaceTransactionBuilder(signer, model::NamespaceType::Child, namespaceName));
		auto namespaceId = pTransaction->NamespaceId;
		auto pPlugin = TTraits::CreatePlugin();

		// Act:
		mappers::bson_stream::document builder;
		pPlugin->streamTransaction(builder, *pTransaction);
		auto view = builder.view();

		// Assert:
		EXPECT_EQ(4u, test::GetFieldCount(view));
		AssertSharedRegisterNamespaceData(pTransaction->NamespaceType, namespaceId, namespaceName, view);
		EXPECT_EQ(pTransaction->ParentId, NamespaceId(test::GetUint64(view, "parentId")));
	}

	// endregion
}}}
