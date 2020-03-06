/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/EndExecuteMapper.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "plugins/txes/supercontract/src/model/EndExecuteTransaction.h"
#include "plugins/txes/supercontract/tests/test/SuperContractTestUtils.h"
#include "tests/test/SuperContractMapperTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS EndExecuteMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(EndExecute,)

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			test::AssertEqualMosaics(transaction, dbTransaction);
			EXPECT_EQ(transaction.OperationToken, test::GetHashValue(dbTransaction, "operationToken"));
			EXPECT_EQ(transaction.Result, test::GetUint16(dbTransaction, "result"));
		}

		template<typename TTraits>
		void AssertCanMapEndExecuteTransaction(size_t numMosaics) {
			// Arrange:
			auto pTransaction = test::CreateEndExecuteTransaction<typename TTraits::TransactionType>(numMosaics);
			auto pPlugin = TTraits::CreatePlugin();

			// Act:
			mappers::bson_stream::document builder;
			pPlugin->streamTransaction(builder, *pTransaction);
			auto view = builder.view();

			// Assert:
			EXPECT_EQ(3u, test::GetFieldCount(view));
			AssertEqualNonInheritedData(*pTransaction, view);
		}
	}

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , model::Entity_Type_EndExecute)

	// region streamTransaction

	PLUGIN_TEST(CanMapEndExecuteTransactionWithoutMosaics) {
		// Assert:
		AssertCanMapEndExecuteTransaction<TTraits>(0);
	}

	PLUGIN_TEST(CanMapEndExecuteTransactionWithMosaics) {
		// Assert:
		AssertCanMapEndExecuteTransaction<TTraits>(3);
	}

	// endregion
}}}
