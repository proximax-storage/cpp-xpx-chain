/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/UploadFileMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "plugins/txes/supercontract/src/model/UploadFileTransaction.h"
#include "plugins/txes/supercontract/tests/test/SuperContractTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS UploadFileMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(UploadFile,)

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			EXPECT_EQ(transaction.DriveKey, test::GetKeyValue(dbTransaction, "driveKey"));
			EXPECT_EQ(transaction.RootHash, test::GetHashValue(dbTransaction, "rootHash"));
			EXPECT_EQ(transaction.XorRootHash, test::GetHashValue(dbTransaction, "xorRootHash"));

			const auto& addActionArray = dbTransaction["addActions"].get_array().value;
			ASSERT_EQ(transaction.AddActionsCount, test::GetFieldCount(addActionArray));
			auto addActionIter = addActionArray.cbegin();
			auto pAddAction = transaction.AddActionsPtr();
			for (uint8_t i = 0; i < transaction.AddActionsCount; ++i, ++pAddAction, ++addActionIter) {
				const auto& array = addActionIter->get_document().view();
				EXPECT_EQ(pAddAction->FileHash, test::GetHashValue(array, "fileHash"));
				EXPECT_EQ(pAddAction->FileSize, test::GetUint64(array, "fileSize"));
			}

			const auto& removeActionArray = dbTransaction["removeActions"].get_array().value;
			ASSERT_EQ(transaction.RemoveActionsCount, test::GetFieldCount(removeActionArray));
			auto removeActionIter = removeActionArray.cbegin();
			auto pRemoveAction = transaction.RemoveActionsPtr();
			for (uint8_t i = 0; i < transaction.RemoveActionsCount; ++i, ++pRemoveAction, ++removeActionIter) {
				EXPECT_EQ(pRemoveAction->FileHash, test::GetHashValue(removeActionIter->get_document().view(), "fileHash"));
			}
		}

		template<typename TTraits>
		void AssertCanMapUploadFileTransaction(size_t numAddActions, size_t numRemoveActions) {
			// Arrange:
			auto pTransaction = test::CreateUploadFileTransaction<typename TTraits::TransactionType>(numAddActions, numRemoveActions);
			auto pPlugin = TTraits::CreatePlugin();

			// Act:
			mappers::bson_stream::document builder;
			pPlugin->streamTransaction(builder, *pTransaction);
			auto view = builder.view();

			// Assert:
			EXPECT_EQ(5u, test::GetFieldCount(view));
			AssertEqualNonInheritedData(*pTransaction, view);
		}
	}

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, model::Entity_Type_UploadFile)

	PLUGIN_TEST(CanMapUploadFileTransactionWithoutOffers) {
		// Assert:
		AssertCanMapUploadFileTransaction<TTraits>(0, 0);
	}

	PLUGIN_TEST(CanMapUploadFileTransactionWithOffers) {
		// Assert:
		AssertCanMapUploadFileTransaction<TTraits>(3, 4);
	}
}}}
