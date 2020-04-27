/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/ResumeMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/supercontract/src/model/ResumeTransaction.h"
#include "plugins/txes/supercontract/tests/test/SuperContractTestUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS ResumeMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(Resume,)

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			EXPECT_EQ(transaction.SuperContract, test::GetKeyValue(dbTransaction, "superContract"));
		}
	}

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , model::Entity_Type_Resume)

	// region streamTransaction

	PLUGIN_TEST(CanMapResumeTransaction) {
		// Arrange:
		auto pTransaction = test::CreateResumeTransaction<typename TTraits::TransactionType>();
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
