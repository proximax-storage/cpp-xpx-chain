/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/PrepareDriveMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "plugins/txes/service/src/model/PrepareDriveTransaction.h"
#include "plugins/txes/service/tests/test/ServiceTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS PrepareDriveMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS_NO_ADAPT(PrepareDrive,)

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			if (transaction.EntityVersion() == 1)
				EXPECT_EQ(transaction.DriveKey, test::GetKeyValue(dbTransaction, "drive"));
			else
				EXPECT_EQ(transaction.Owner, test::GetKeyValue(dbTransaction, "owner"));
			EXPECT_EQ(transaction.Duration.unwrap(), test::GetUint64(dbTransaction, "duration"));
			EXPECT_EQ(transaction.BillingPeriod.unwrap(), test::GetUint64(dbTransaction, "billingPeriod"));
			EXPECT_EQ(transaction.BillingPrice.unwrap(), test::GetUint64(dbTransaction, "billingPrice"));
			EXPECT_EQ(transaction.DriveSize, test::GetUint64(dbTransaction, "driveSize"));
			EXPECT_EQ(transaction.Replicas, test::GetUint16(dbTransaction, "replicas"));
			EXPECT_EQ(transaction.MinReplicators, test::GetUint16(dbTransaction, "minReplicators"));
			EXPECT_EQ(transaction.PercentApprovers, test::GetUint8(dbTransaction, "percentApprovers"));
		}

		template<typename TTraits>
		void AssertCanMapPrepareDriveTransaction(VersionType version) {
			// Arrange:
			auto pTransaction = test::CreatePrepareDriveTransaction<typename TTraits::TransactionType>(version);
			auto pPlugin = TTraits::CreatePlugin();

			// Act:
			mappers::bson_stream::document builder;
			pPlugin->streamTransaction(builder, *pTransaction);
			auto view = builder.view();

			// Assert:
			EXPECT_EQ(8u, test::GetFieldCount(view));
			AssertEqualNonInheritedData(*pTransaction, view);
		}
	}

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, model::Entity_Type_PrepareDrive)

	PLUGIN_TEST(CanMapPrepareDriveTransaction_v1) {
		// Assert:
		AssertCanMapPrepareDriveTransaction<TTraits>(1);
	}

	PLUGIN_TEST(CanMapPrepareDriveTransaction_v2) {
		// Assert:
		AssertCanMapPrepareDriveTransaction<TTraits>(2);
	}
}}}
