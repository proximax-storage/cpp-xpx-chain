/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/CatapultConfigMapper.h"
#include "sdk/src/builders/CatapultConfigBuilder.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/constants.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS CatapultConfigMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS(CatapultConfig)

		auto CreateCatapultConfigTransactionBuilder(
				const Key& signer,
				const BlockDuration& applyHeightDelta,
				const std::string& blockChainConfig,
				const std::string& supportedEntityVersions) {
			builders::CatapultConfigBuilder builder(model::NetworkIdentifier::Mijin_Test, signer);
			builder.setApplyHeightDelta(applyHeightDelta);
			builder.setBlockChainConfig(utils::RawBuffer(reinterpret_cast<const uint8_t*>(blockChainConfig.data()), blockChainConfig.size()));
			builder.setSupportedVersionsConfig(utils::RawBuffer(reinterpret_cast<const uint8_t*>(supportedEntityVersions.data()), supportedEntityVersions.size()));

			return builder;
		}

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			EXPECT_EQ(transaction.ApplyHeightDelta.unwrap(), test::GetInt64(dbTransaction, "applyHeightDelta"));
			auto blockChainConfig = std::string((const char*)transaction.BlockChainConfigPtr(), transaction.BlockChainConfigSize);
			EXPECT_EQ(blockChainConfig, dbTransaction["blockChainConfig"].get_utf8().value.to_string());
			auto supportedEntityVersions = std::string((const char*)transaction.SupportedEntityVersionsPtr(), transaction.SupportedEntityVersionsSize);
			EXPECT_EQ(supportedEntityVersions, dbTransaction["supportedEntityVersions"].get_utf8().value.to_string());
		}

		template<typename TTraits>
		void AssertCanMapCatapultConfigTransaction(
				const BlockDuration& applyHeightDelta,
				const std::string& blockChainConfig,
				const std::string& supportedEntityVersions) {
			// Arrange:
			auto signer = test::GenerateRandomByteArray<Key>();
			auto pBuilder = CreateCatapultConfigTransactionBuilder(signer, applyHeightDelta, blockChainConfig, supportedEntityVersions);
			auto pTransaction = TTraits::Adapt(pBuilder);
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

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, model::Entity_Type_Catapult_Config)

	// region streamTransaction

	PLUGIN_TEST(CanMapCatapultConfigTransactionWithEmptyValues) {
		// Assert:
		AssertCanMapCatapultConfigTransaction<TTraits>(BlockDuration(), std::string(), std::string());
	}

	PLUGIN_TEST(CanMapCatapultConfigTransactionWithNonEmptyValues) {
		// Assert:
		AssertCanMapCatapultConfigTransaction<TTraits>(BlockDuration(100), "aaa", "bbb");
	}

	// endregion
}}}
