/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/BlockchainUpgradeMapper.h"
#include "sdk/src/builders/BlockchainUpgradeBuilder.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/constants.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS BlockchainUpgradeMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS(BlockchainUpgrade)

		auto CreateBlockchainUpgradeTransactionBuilder(
				const Key& signer,
				const BlockDuration& upgradePeriod,
				const BlockchainVersion& blockChainVersion) {
			builders::BlockchainUpgradeBuilder builder(model::NetworkIdentifier::Mijin_Test, signer);
			builder.setUpgradePeriod(upgradePeriod);
			builder.setNewBlockchainVersion(blockChainVersion);

			return builder;
		}

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			EXPECT_EQ(transaction.UpgradePeriod.unwrap(), test::GetInt64(dbTransaction, "upgradePeriod"));
			EXPECT_EQ(transaction.NewBlockchainVersion.unwrap(), test::GetInt64(dbTransaction, "newBlockchainVersion"));
		}

		template<typename TTraits>
		void AssertCanMapBlockchainUpgradeTransaction(
				const BlockDuration& upgradePeriod,
				const BlockchainVersion& blockChainVersion) {
			// Arrange:
			auto signer = test::GenerateRandomByteArray<Key>();
			auto pBuilder = CreateBlockchainUpgradeTransactionBuilder(signer, upgradePeriod, blockChainVersion);
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

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, model::Entity_Type_Blockchain_Upgrade)

	// region streamTransaction

	PLUGIN_TEST(CanMapBlockchainUpgradeTransactionWithEmptyValues) {
		// Assert:
		AssertCanMapBlockchainUpgradeTransaction<TTraits>(BlockDuration(), BlockchainVersion());
	}

	PLUGIN_TEST(CanMapBlockchainUpgradeTransactionWithNonEmptyValues) {
		// Assert:
		AssertCanMapBlockchainUpgradeTransaction<TTraits>(BlockDuration(100), BlockchainVersion(7));
	}

	// endregion
}}}
