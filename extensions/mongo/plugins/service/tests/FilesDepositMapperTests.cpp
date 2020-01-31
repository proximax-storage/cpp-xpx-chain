/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/FilesDepositMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "plugins/txes/service/src/model/FilesDepositTransaction.h"
#include "plugins/txes/service/tests/test/ServiceTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS FilesDepositMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(FilesDeposit,)

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			EXPECT_EQ(transaction.DriveKey, test::GetKeyValue(dbTransaction, "driveKey"));

			const auto& fileArray = dbTransaction["files"].get_array().value;
			ASSERT_EQ(transaction.FilesCount, test::GetFieldCount(fileArray));
			auto fileIter = fileArray.cbegin();
			const auto* pFile = transaction.FilesPtr();
			for (uint8_t i = 0; i < transaction.FilesCount; ++i, ++pFile, ++fileIter) {
				EXPECT_EQ(pFile->FileHash, test::GetHashValue(fileIter->get_document().view(), "fileHash"));
			}
		}

		template<typename TTraits>
		void AssertCanMapFilesDepositTransaction(size_t numFiles) {
			// Arrange:
			auto pTransaction = test::CreateFilesDepositTransaction<typename TTraits::TransactionType>(numFiles);
			auto pPlugin = TTraits::CreatePlugin();

			// Act:
			mappers::bson_stream::document builder;
			pPlugin->streamTransaction(builder, *pTransaction);
			auto view = builder.view();

			// Assert:
			EXPECT_EQ(2u, test::GetFieldCount(view));
			AssertEqualNonInheritedData(*pTransaction, view);
		}
	}

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, model::Entity_Type_FilesDeposit)

	PLUGIN_TEST(CanMapFilesDepositTransactionWithoutFiles) {
		// Assert:
		AssertCanMapFilesDepositTransaction<TTraits>(0);
	}

	PLUGIN_TEST(CanMapFilesDepositTransactionWithFiles) {
		// Assert:
		AssertCanMapFilesDepositTransaction<TTraits>(3);
	}
}}}
