/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/EndOperationMapper.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "plugins/txes/operation/src/model/EndOperationTransaction.h"
#include "plugins/txes/operation/tests/test/OperationTestUtils.h"
#include "tests/test/OperationMapperTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS EndOperationMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(EndOperation,)

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			test::AssertEqualMosaics(transaction, dbTransaction);
			EXPECT_EQ(transaction.OperationToken, test::GetHashValue(dbTransaction, "operationToken"));
			EXPECT_EQ(transaction.Result, test::GetUint16(dbTransaction, "result"));
		}

		template<typename TTraits>
		void AssertCanMapEndOperationTransaction(size_t numMosaics) {
			// Arrange:
			auto pTransaction = test::CreateEndOperationTransaction<typename TTraits::TransactionType>(numMosaics);
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

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , model::Entity_Type_EndOperation)

	// region streamTransaction

	PLUGIN_TEST(CanMapEndOperationTransactionWithoutMosaics) {
		// Assert:
		AssertCanMapEndOperationTransaction<TTraits>(0);
	}

	PLUGIN_TEST(CanMapEndOperationTransactionWithMosaics) {
		// Assert:
		AssertCanMapEndOperationTransaction<TTraits>(3);
	}

	// endregion
}}}
