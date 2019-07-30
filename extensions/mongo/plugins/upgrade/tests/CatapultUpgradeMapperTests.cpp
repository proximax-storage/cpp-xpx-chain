/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/CatapultUpgradeMapper.h"
#include "sdk/src/builders/CatapultUpgradeBuilder.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/constants.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS CatapultUpgradeMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS(CatapultUpgrade)

		auto CreateCatapultUpgradeTransactionBuilder(
				const Key& signer,
				const BlockDuration& upgradePeriod,
				const CatapultVersion& catapultVersion) {
			builders::CatapultUpgradeBuilder builder(model::NetworkIdentifier::Mijin_Test, signer);
			builder.setUpgradePeriod(upgradePeriod);
			builder.setNewCatapultVersion(catapultVersion);

			return builder;
		}

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			EXPECT_EQ(transaction.UpgradePeriod.unwrap(), test::GetInt64(dbTransaction, "upgradePeriod"));
			EXPECT_EQ(transaction.NewCatapultVersion.unwrap(), test::GetInt64(dbTransaction, "newCatapultVersion"));
		}

		template<typename TTraits>
		void AssertCanMapCatapultUpgradeTransaction(
				const BlockDuration& upgradePeriod,
				const CatapultVersion& catapultVersion) {
			// Arrange:
			auto signer = test::GenerateRandomByteArray<Key>();
			auto pBuilder = CreateCatapultUpgradeTransactionBuilder(signer, upgradePeriod, catapultVersion);
			auto pTransaction = TTraits::Adapt(pBuilder);
			auto pPlugin = TTraits::CreatePlugin();

			// Act:
			mappers::bson_stream::document builder;
			pPlugin->streamTransaction(builder, *pTransaction);
			auto view = builder.view();

			// Assert:
			EXPECT_EQ(2u, test::GetFieldCount(view));
			AssertEqualNonInheritedData(*pTransaction, view);
		}
	}

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, model::Entity_Type_Catapult_Upgrade)

	// region streamTransaction

	PLUGIN_TEST(CanMapCatapultUpgradeTransactionWithEmptyValues) {
		// Assert:
		AssertCanMapCatapultUpgradeTransaction<TTraits>(BlockDuration(), CatapultVersion());
	}

	PLUGIN_TEST(CanMapCatapultUpgradeTransactionWithNonEmptyValues) {
		// Assert:
		AssertCanMapCatapultUpgradeTransaction<TTraits>(BlockDuration(100), CatapultVersion(7));
	}

	// endregion
}}}
