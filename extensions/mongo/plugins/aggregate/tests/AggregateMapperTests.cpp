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

#include "src/AggregateMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/aggregate/src/model/AggregateTransaction.h"
#include "catapult/model/EntityHasher.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "mongo/tests/test/mocks/MockTransactionMapper.h"
#include "tests/test/core/AddressTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS AggregateMapperTests

	using TransactionType = model::AggregateTransaction;
	using EmbeddedTransactionType = mocks::EmbeddedMockTransaction;

	namespace {
		constexpr auto Entity_Type = static_cast<model::EntityType>(9876);
		auto Immutable_Config = config::ImmutableConfiguration::Uninitialized();

		auto AllocateAggregateTransaction(uint16_t numTransactions, uint16_t numCosignatures) {
			uint32_t entitySize = sizeof(TransactionType)
					+ numTransactions * sizeof(EmbeddedTransactionType)
					+ numCosignatures * sizeof(model::Cosignature);
			auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
			pTransaction->Size = entitySize;
			pTransaction->PayloadSize = numTransactions * sizeof(EmbeddedTransactionType);
			return pTransaction;
		}
	}

	// region basic

	TEST(TEST_CLASS, CanCreatePlugin) {
		// Act:
		MongoTransactionRegistry registry;
		auto pPlugin = CreateAggregateTransactionMongoPlugin(registry, Immutable_Config, Entity_Type);

		// Assert:
		EXPECT_EQ(Entity_Type, pPlugin->type());
	}

	TEST(TEST_CLASS, PluginDoesNotSupportEmbedding) {
		// Arrange:
		MongoTransactionRegistry registry;
		auto pPlugin = CreateAggregateTransactionMongoPlugin(registry, Immutable_Config, Entity_Type);

		// Act + Assert:
		EXPECT_FALSE(pPlugin->supportsEmbedding());
		EXPECT_THROW(pPlugin->embeddedPlugin(), catapult_runtime_error);
	}

	// endregion

	// region streamTransaction

	namespace {
		void AssertCanMapAggregateTransactionWithCosignatures(uint16_t numCosignatures) {
			// Arrange: create aggregate with a single sub transaction
			auto pTransaction = AllocateAggregateTransaction(1, numCosignatures);

			// - create and copy cosignatures
			auto cosignatures = test::GenerateRandomDataVector<model::Cosignature>(numCosignatures);
			std::memcpy(
					static_cast<void*>(pTransaction->CosignaturesPtr()),
					cosignatures.data(),
					numCosignatures * sizeof(model::Cosignature));

			// - create the plugin
			MongoTransactionRegistry registry;
			auto pPlugin = CreateAggregateTransactionMongoPlugin(registry, Immutable_Config, Entity_Type);

			// Act:
			mappers::bson_stream::document builder;
			pPlugin->streamTransaction(builder, *pTransaction);
			auto view = builder.view();

			// Assert: only cosignatures should be present and they should always be present (even if there are no cosignatures)
			EXPECT_EQ(1u, test::GetFieldCount(view));

			auto dbCosignatures = view["cosignatures"].get_array().value;
			ASSERT_EQ(numCosignatures, test::GetFieldCount(dbCosignatures));

			test::AssertEqualCosignatures(cosignatures, dbCosignatures);
		}
	}

	TEST(TEST_CLASS, CanMapAggregateTransactionWithoutCosignatures) {
		// Assert:
		AssertCanMapAggregateTransactionWithCosignatures(0);
	}

	TEST(TEST_CLASS, CanMapAggregateTransactionWithSingleCosignature) {
		// Assert:
		AssertCanMapAggregateTransactionWithCosignatures(1);
	}

	TEST(TEST_CLASS, CanMapAggregateTransactionWithMultipleCosignatures) {
		// Assert:
		AssertCanMapAggregateTransactionWithCosignatures(3);
	}

	// endregion

	// region extractDependentDocuments

	namespace {
		void AssertExtractDependentDocuments(uint16_t numTransactions) {
			// Arrange: create aggregate with two cosignatures
			auto pTransaction = AllocateAggregateTransaction(numTransactions, 2);

			// - create and copy sub transactions
			auto subTransactions = test::GenerateRandomDataVector<EmbeddedTransactionType>(numTransactions);
			for (auto& subTransaction : subTransactions) {
				subTransaction.Size = sizeof(EmbeddedTransactionType);
				subTransaction.Type = EmbeddedTransactionType::Entity_Type;
				subTransaction.Data.Size = 0;
			}

			auto transactionsSize = numTransactions * sizeof(EmbeddedTransactionType);
			std::memcpy(static_cast<void*>(pTransaction->TransactionsPtr()), subTransactions.data(), transactionsSize);

			// - create the plugin
			MongoTransactionRegistry registry;
			registry.registerPlugin(mocks::CreateMockTransactionMongoPlugin());
			auto pPlugin = CreateAggregateTransactionMongoPlugin(registry, Immutable_Config, Entity_Type);

			// Act:
			model::TransactionElement transactionElement(*pTransaction);
			transactionElement.EntityHash = test::GenerateRandomByteArray<Hash256>();
			transactionElement.MerkleComponentHash = test::GenerateRandomByteArray<Hash256>();
			transactionElement.OptionalExtractedAddresses = test::GenerateRandomUnresolvedAddressSetPointer(3);
			auto metadata = MongoTransactionMetadata(transactionElement, Height(12), 2);
			auto documents = pPlugin->extractDependentDocuments(*pTransaction, metadata);

			utils::Mempool pool;

			// Assert:
			ASSERT_EQ(numTransactions, documents.size());

			for (auto i = 0u; i < numTransactions; ++i) {
				const auto& subTransaction = subTransactions[i];

				auto uniqueAggregateHash = model::CalculateHash(
					catapult::plugins::ConvertEmbeddedTransaction(subTransaction, pTransaction->Deadline, pool),
					Immutable_Config.GenerationHash
				);

				// - the document has meta and transaction parts
				auto view = documents[i].view();
				EXPECT_EQ(2u, test::GetFieldCount(view));

				auto metaView = view["meta"].get_document().view();
				EXPECT_EQ(5u, test::GetFieldCount(metaView));
				EXPECT_EQ(Height(12), Height(test::GetUint64(metaView, "height")));
				EXPECT_EQ(metadata.EntityHash, test::GetHashValue(metaView, "aggregateHash"));
				EXPECT_EQ(uniqueAggregateHash, test::GetHashValue(metaView, "uniqueAggregateHash"));
				EXPECT_EQ(metadata.ObjectId, metaView["aggregateId"].get_oid().value);
				EXPECT_EQ(i, test::GetUint32(metaView, "index"));

				// - the mock mapper adds a recipient entry
				auto subTransactionView = view["transaction"].get_document().view();
				EXPECT_EQ(4u, test::GetFieldCount(subTransactionView));
				test::AssertEqualEmbeddedTransactionData(subTransaction, subTransactionView);
				EXPECT_EQ(subTransaction.Recipient, test::GetKeyValue(subTransactionView, "recipient"));
			}
		}
	}

	TEST(TEST_CLASS, NoDependentDocumentsAreExtractedWhenThereAreNoSubTransactions) {
		// Assert:
		AssertExtractDependentDocuments(0);
	}

	TEST(TEST_CLASS, SingleDependentDocumentIsExtractedWhenThereIsOneSubTransaction) {
		// Assert:
		AssertExtractDependentDocuments(1);
	}

	TEST(TEST_CLASS, MultipleDependentDocumentsAreExtractedWhenThereAreMultipleSubTransactions) {
		// Assert:
		AssertExtractDependentDocuments(3);
	}

	// endregion
}}}
