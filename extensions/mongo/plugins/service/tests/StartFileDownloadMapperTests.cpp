/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/StartFileDownloadMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "plugins/txes/service/src/model/StartFileDownloadTransaction.h"
#include "plugins/txes/service/tests/test/ServiceTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS StartFileDownloadMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(StartFileDownload,)

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			EXPECT_EQ(transaction.DriveKey, test::GetKeyValue(dbTransaction, "driveKey"));

			const auto& fileArray = dbTransaction["files"].get_array().value;
			ASSERT_EQ(transaction.FileCount, test::GetFieldCount(fileArray));
			auto fileIter = fileArray.cbegin();
			auto pFile = transaction.FilesPtr();
			for (uint8_t i = 0; i < transaction.FileCount; ++i, ++pFile, ++fileIter) {
				const auto& dbFile = fileIter->get_document().view();
				EXPECT_EQ(pFile->FileHash, test::GetHashValue(dbFile, "fileHash"));
				EXPECT_EQ(pFile->FileSize, test::GetUint64(dbFile, "fileSize"));
			}
		}

		template<typename TTraits>
		void AssertCanMapStartFileDownloadTransaction(size_t numFiles) {
			// Arrange:
			auto pTransaction = test::CreateStartFileDownloadTransaction<typename TTraits::TransactionType>(numFiles);
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

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, model::Entity_Type_StartFileDownload)

	PLUGIN_TEST(CanMapStartFileDownloadTransactionWithoutOffers) {
		// Assert:
		AssertCanMapStartFileDownloadTransaction<TTraits>(0);
	}

	PLUGIN_TEST(CanMapStartFileDownloadTransactionWithOffers) {
		// Assert:
		AssertCanMapStartFileDownloadTransaction<TTraits>(3);
	}
}}}
