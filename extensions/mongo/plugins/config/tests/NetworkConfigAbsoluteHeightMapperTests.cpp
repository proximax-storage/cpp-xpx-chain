/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/NetworkConfigMapper.h"
#include "sdk/src/builders/NetworkConfigBuilder.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/constants.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS NetworkConfigAbsoluteHeightMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS(NetworkConfigAbsoluteHeight)

		auto CreateNetworkConfigTransactionBuilder(
				const Key& signer,
				const Height& applyHeight,
				const std::string& networkConfig,
				const std::string& supportedEntityVersions) {
			builders::NetworkConfigBuilder<model::NetworkConfigAbsoluteHeightTransaction> builder(model::NetworkIdentifier::Mijin_Test, signer);
			builder.setApplyHeight(applyHeight);
			builder.setBlockChainConfig(utils::RawBuffer(reinterpret_cast<const uint8_t*>(networkConfig.data()), networkConfig.size()));
			builder.setSupportedVersionsConfig(utils::RawBuffer(reinterpret_cast<const uint8_t*>(supportedEntityVersions.data()), supportedEntityVersions.size()));

			return builder;
		}

		template<typename TTransaction>
		void AssertEqualNonInheritedData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			EXPECT_EQ(transaction.ApplyHeight.unwrap(), test::GetInt64(dbTransaction, "applyHeight"));
			auto networkConfig = std::string((const char*)transaction.BlockChainConfigPtr(), transaction.BlockChainConfigSize);
			EXPECT_EQ(networkConfig, std::string(dbTransaction["networkConfig"].get_utf8().value));
			auto supportedEntityVersions = std::string((const char*)transaction.SupportedEntityVersionsPtr(), transaction.SupportedEntityVersionsSize);
			EXPECT_EQ(supportedEntityVersions, std::string(dbTransaction["supportedEntityVersions"].get_utf8().value));
		}

		template<typename TTraits>
		void AssertCanMapNetworkConfigTransaction(
				const Height& applyHeight,
				const std::string& networkConfig,
				const std::string& supportedEntityVersions) {
			// Arrange:
			auto signer = test::GenerateRandomByteArray<Key>();
			auto pBuilder = CreateNetworkConfigTransactionBuilder(signer, applyHeight, networkConfig, supportedEntityVersions);
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

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, model::Entity_Type_Network_Config_Absolute_Height)

	// region streamTransaction

	PLUGIN_TEST(CanMapNetworkConfigTransactionWithEmptyValues) {
		// Assert:
		AssertCanMapNetworkConfigTransaction<TTraits>(Height(), std::string(), std::string());
	}

	PLUGIN_TEST(CanMapNetworkConfigTransactionWithNonEmptyValues) {
		// Assert:
		AssertCanMapNetworkConfigTransaction<TTraits>(Height(100), "aaa", "bbb");
	}

	// endregion
}}}
