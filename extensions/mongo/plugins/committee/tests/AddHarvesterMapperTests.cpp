/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/AddHarvesterMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "plugins/txes/committee/src/model/AddHarvesterTransaction.h"
#include "plugins/txes/committee/tests/test/CommitteeTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS AddHarvesterMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS(AddHarvester)
	}

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, model::Entity_Type_AddHarvester)

	// region streamTransaction

	PLUGIN_TEST(CanMapAddHarvesterTransaction) {
		// Arrange:
		auto pTransaction = test::CreateHarvesterTransaction<typename TTraits::TransactionType>();
		auto pPlugin = TTraits::CreatePlugin();

		// Act:
		mappers::bson_stream::document builder;
		pPlugin->streamTransaction(builder, *pTransaction);
		auto view = builder.view();

		// Assert:
		EXPECT_EQ(1u, test::GetFieldCount(view));
	}

	// endregion
}}}
