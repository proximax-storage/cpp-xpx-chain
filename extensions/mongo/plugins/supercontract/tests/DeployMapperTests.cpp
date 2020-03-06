/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/DeployMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/supercontract/src/model/DeployTransaction.h"
#include "plugins/txes/supercontract/tests/test/SuperContractTestUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS DeployMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(Deploy,)

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			EXPECT_EQ(transaction.DriveKey, test::GetKeyValue(dbTransaction, "drive"));
			EXPECT_EQ(transaction.Owner, test::GetKeyValue(dbTransaction, "owner"));
			EXPECT_EQ(transaction.FileHash, test::GetHashValue(dbTransaction, "fileHash"));
			EXPECT_EQ(transaction.VmVersion.unwrap(), test::GetUint64(dbTransaction, "vmVersion"));
		}
	}

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , model::Entity_Type_Deploy)

	// region streamTransaction

	PLUGIN_TEST(CanMapDeployTransaction) {
		// Arrange:
		auto pTransaction = test::CreateDeployTransaction<typename TTraits::TransactionType>();
		auto pPlugin = TTraits::CreatePlugin();

		// Act:
		mappers::bson_stream::document builder;
		pPlugin->streamTransaction(builder, *pTransaction);
		auto view = builder.view();

		// Assert:
		EXPECT_EQ(4u, test::GetFieldCount(view));
		AssertEqualNonInheritedData(*pTransaction, view);
	}

	// endregion
}}}
