/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/DriveFilesRewardMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "plugins/txes/service/src/model/DriveFilesRewardTransaction.h"
#include "plugins/txes/service/tests/test/ServiceTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS DriveFilesRewardMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(DriveFilesReward,)

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			const auto& uploadInfoArray = dbTransaction["uploadInfos"].get_array().value;
			ASSERT_EQ(transaction.UploadInfosCount, test::GetFieldCount(uploadInfoArray));
			auto uploadInfoIter = uploadInfoArray.cbegin();
			const auto* pFailure = transaction.UploadInfosPtr();
			for (uint16_t i = 0; i < transaction.UploadInfosCount; ++i, ++pFailure, ++uploadInfoIter) {
				const auto& array = uploadInfoIter->get_document().view();
				EXPECT_EQ(pFailure->Participant, test::GetKeyValue(array, "participant"));
				EXPECT_EQ(pFailure->Uploaded, test::GetUint64(array, "uploaded"));
			}
		}

		template<typename TTraits>
		void AssertCanMapDriveFilesRewardTransaction(size_t numUploadInfos) {
			// Arrange:
			auto pTransaction = test::CreateDriveFilesRewardTransaction<typename TTraits::TransactionType>(numUploadInfos);
			auto pPlugin = TTraits::CreatePlugin();

			// Act:
			mappers::bson_stream::document builder;
			pPlugin->streamTransaction(builder, *pTransaction);
			auto view = builder.view();

			// Assert:
			EXPECT_EQ(1u, test::GetFieldCount(view));
			AssertEqualNonInheritedData(*pTransaction, view);
		}
	}

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, model::Entity_Type_DriveFilesReward)

	PLUGIN_TEST(CanMapDriveFilesRewardTransactionWithoutUploadInfos) {
		// Assert:
		AssertCanMapDriveFilesRewardTransaction<TTraits>(0);
	}

	PLUGIN_TEST(CanMapDriveFilesRewardTransactionWithUploadInfos) {
		// Assert:
		AssertCanMapDriveFilesRewardTransaction<TTraits>(3);
	}
}}}
