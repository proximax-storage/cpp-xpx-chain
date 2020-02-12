/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/OperationIdentifyMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/operation/src/model/OperationIdentifyTransaction.h"
#include "plugins/txes/operation/tests/test/OperationTestUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS OperationIdentifyMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(OperationIdentify,)

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			EXPECT_EQ(transaction.OperationToken, test::GetHashValue(dbTransaction, "operationToken"));
		}
	}

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , model::Entity_Type_OperationIdentify)

	// region streamTransaction

	PLUGIN_TEST(CanMapOperationIdentifyTransaction) {
		// Arrange:
		auto pTransaction = test::CreateOperationIdentifyTransaction<typename TTraits::TransactionType>();
		auto pPlugin = TTraits::CreatePlugin();

		// Act:
		mappers::bson_stream::document builder;
		pPlugin->streamTransaction(builder, *pTransaction);
		auto view = builder.view();

		// Assert:
		EXPECT_EQ(1u, test::GetFieldCount(view));
		AssertEqualNonInheritedData(*pTransaction, view);
	}

	// endregion
}}}
