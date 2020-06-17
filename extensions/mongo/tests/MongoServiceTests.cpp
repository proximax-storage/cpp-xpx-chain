/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/src/MongoService.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTestUtils.h"
#include "tests/test/local/ServiceLocatorTestContext.h"
#include "tests/test/local/ServiceTestUtils.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo {

#define TEST_CLASS MongoServiceTests

	namespace {
		constexpr auto Transaction_Statuses_Collection = "transactionStatuses";

		auto ResetDatabaseAndCreateMongoContext() {
			test::ResetDatabase(test::DatabaseName());
			return test::CreateDefaultMongoStorageContext(test::DatabaseName());
		}

		struct MongoServiceTraits {
			static auto CreateRegistrar() {
				auto pContext = utils::UniqueToShared(ResetDatabaseAndCreateMongoContext());
				auto pTransactionRegistry = std::make_shared<const mongo::MongoTransactionRegistry>(test::CreateDefaultMongoTransactionRegistry());
				return CreateMongoServiceRegistrar(pContext, pTransactionRegistry);
			}
		};

		using TestContext = test::ServiceLocatorTestContext<MongoServiceTraits>;

		void AddTransactionStatuses(const std::vector<Hash256>& transactionHashes) {
			auto connection = test::CreateDbConnection();
			auto database = connection[test::DatabaseName()];
			auto collection = database[Transaction_Statuses_Collection];

			int32_t status = 0;
			for (const auto& hash : transactionHashes) {
				auto doc = document()
					<< "hash" << mappers::ToBinary(hash)
					<< "status" << status
					<< "deadline" << static_cast<int64_t>(status * 10)
					<< finalize;

				collection.insert_one(doc.view()).get();
				status++;
			}
		}

		void AssertTransactionStatus(const Hash256& expectedTransactionStatus) {
			auto connection = test::CreateDbConnection();
			auto database = connection[test::DatabaseName()];
			auto collection = database[Transaction_Statuses_Collection];

			// Assert:
			EXPECT_EQ(1u, static_cast<size_t>(collection.count_documents({})));
			auto txCursor = collection.find({});
			EXPECT_EQ(expectedTransactionStatus, test::GetHashValue(*txCursor.begin(), "hash"));
		}
	}

	// region MongoService basic

	ADD_SERVICE_REGISTRAR_INFO_TEST(Mongo, Initial_With_Modules)

	TEST(TEST_CLASS, MongoServicesAreRegistered) {
		// Arrange:
		TestContext context;

		// Act:
		context.boot();

		// Assert:
		EXPECT_EQ(1u, context.locator().numServices());
		EXPECT_EQ(0u, context.locator().counters().size());
	}

	// endregion

	// region MongoService hooks (TransactionsChangeHandler)

	TEST(TEST_CLASS, TransactionsChangeHandlerRemovesConfirmedTransactionsFromCache) {
		// Arrange:
		TestContext context;
		context.boot();
		std::vector<Hash256> transactionHashes{
			test::GenerateRandomByteArray<Hash256>(),
			test::GenerateRandomByteArray<Hash256>(),
			test::GenerateRandomByteArray<Hash256>(),
		};
		AddTransactionStatuses(transactionHashes);

		// Act: trigger deletions of two statuses.
		auto handler = context.testState().state().hooks().transactionsChangeHandler();
		utils::HashPointerSet addedTransactionHashes{
			&transactionHashes[0],
			&transactionHashes[1],
		};
		std::vector<model::TransactionInfo> revertedTransactionInfos;
		handler(consumers::TransactionsChangeInfo(addedTransactionHashes, revertedTransactionInfos));

		// Assert:
		AssertTransactionStatus(transactionHashes[2]);
	}

	// endregion
}}
