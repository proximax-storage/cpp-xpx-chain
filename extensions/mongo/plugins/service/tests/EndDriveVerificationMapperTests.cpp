/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/EndDriveVerificationMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "plugins/txes/service/src/model/EndDriveVerificationTransaction.h"
#include "plugins/txes/service/tests/test/ServiceTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS EndDriveVerificationMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(EndDriveVerification,)

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			const auto& failureArray = dbTransaction["verificationFailures"].get_array().value;
			ASSERT_EQ(transaction.FailureCount, test::GetFieldCount(failureArray));
			auto failureIter = failureArray.cbegin();
			const auto* pFailure = transaction.FailuresPtr();
			for (uint8_t i = 0; i < transaction.FailureCount; ++i, ++pFailure, ++failureIter) {
				const auto& array = failureIter->get_document().view();
				EXPECT_EQ(pFailure->Replicator, test::GetKeyValue(array, "replicator"));
				EXPECT_EQ(pFailure->BlockHash, test::GetHashValue(array, "blockHash"));
			}
		}

		template<typename TTraits>
		void AssertCanMapEndDriveVerificationTransaction(size_t numFailures) {
			// Arrange:
			auto pTransaction = test::CreateEndDriveVerificationTransaction<typename TTraits::TransactionType>(numFailures);
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

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, model::Entity_Type_End_Drive_Verification)

	PLUGIN_TEST(CanMapEndDriveVerificationTransactionWithoutFailures) {
		// Assert:
		AssertCanMapEndDriveVerificationTransaction<TTraits>(0);
	}

	PLUGIN_TEST(CanMapEndDriveVerificationTransactionWithFailures) {
		// Assert:
		AssertCanMapEndDriveVerificationTransaction<TTraits>(3);
	}
}}}
