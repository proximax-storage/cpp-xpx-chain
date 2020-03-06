/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/StartExecuteMapper.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "plugins/txes/supercontract/src/model/StartExecuteTransaction.h"
#include "plugins/txes/supercontract/tests/test/SuperContractTestUtils.h"
#include "tests/test/SuperContractMapperTestUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS StartExecuteMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(StartExecute,)

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			test::AssertEqualMosaics(transaction, dbTransaction);
			EXPECT_EQ(transaction.SuperContract, test::GetKeyValue(dbTransaction, "superContract"));
			if (transaction.FunctionPtr())
				EXPECT_EQ_MEMORY(transaction.FunctionPtr(), test::GetBinary(dbTransaction, "function"), transaction.FunctionSize);
			if (transaction.DataPtr())
				EXPECT_EQ_MEMORY(transaction.DataPtr(), test::GetBinary(dbTransaction, "data"), transaction.DataSize);
		}

		template<typename TTraits>
		void AssertCanMapStartExecuteTransaction(size_t numMosaics, size_t functionSize, size_t dataSize) {
			// Arrange:
			auto pTransaction = test::CreateStartExecuteTransaction<typename TTraits::TransactionType>(numMosaics, functionSize, dataSize);
			auto pPlugin = TTraits::CreatePlugin();

			// Act:
			mappers::bson_stream::document builder;
			pPlugin->streamTransaction(builder, *pTransaction);
			auto view = builder.view();

			// Assert:
			ASSERT_EQ(2u + (functionSize ? 1 : 0) + (dataSize ? 1 : 0), test::GetFieldCount(view));
			AssertEqualNonInheritedData(*pTransaction, view);
		}
	}

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , model::Entity_Type_StartExecute)

	// region streamTransaction

	PLUGIN_TEST(CanMapStartExecuteTransactionWithoutAttachments) {
		// Assert:
		AssertCanMapStartExecuteTransaction<TTraits>(0, 0, 0);
	}

	PLUGIN_TEST(CanMapStartExecuteTransactionWithAttachments) {
		// Assert:
		AssertCanMapStartExecuteTransaction<TTraits>(3, 4, 5);
	}

	// endregion
}}}
