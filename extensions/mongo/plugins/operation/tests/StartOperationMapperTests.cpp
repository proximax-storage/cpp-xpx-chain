/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/StartOperationMapper.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "plugins/txes/operation/src/model/StartOperationTransaction.h"
#include "plugins/txes/operation/tests/test/OperationTestUtils.h"
#include "tests/test/OperationMapperTestUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS StartOperationMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(StartOperation,)

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			test::AssertEqualMosaics(transaction, dbTransaction);
			EXPECT_EQ(transaction.Duration.unwrap(), test::GetUint64(dbTransaction, "duration"));

			const auto& executorArray = dbTransaction["executors"].get_array().value;
			ASSERT_EQ(transaction.ExecutorCount, test::GetFieldCount(executorArray));
			auto pExecutor = transaction.ExecutorsPtr();
			for (const auto& dbExecutor : executorArray) {
				Key executor;
				DbBinaryToModelArray(executor, dbExecutor.get_binary());
				EXPECT_EQ(*pExecutor++, executor);
			}
		}

		template<typename TTraits>
		void AssertCanMapStartOperationTransaction(size_t numMosaics, size_t numExecutors) {
			// Arrange:
			auto pTransaction = test::CreateStartOperationTransaction<typename TTraits::TransactionType>(numMosaics, numExecutors);
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

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , model::Entity_Type_StartOperation)

	// region streamTransaction

	PLUGIN_TEST(CanMapStartOperationTransactionWithoutAttachments) {
		// Assert:
		AssertCanMapStartOperationTransaction<TTraits>(0, 0);
	}

	PLUGIN_TEST(CanMapStartOperationTransactionWithAttachments) {
		// Assert:
		AssertCanMapStartOperationTransaction<TTraits>(3, 4);
	}

	// endregion
}}}
