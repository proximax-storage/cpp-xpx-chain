/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
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
