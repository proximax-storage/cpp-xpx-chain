/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/io/RawFile.h"
#include "src/builders/NetworkConfigBuilder.h"
#include "tests/builders/test/BuilderTestUtils.h"
#include "tests/test/nodeps/Filesystem.h"

namespace catapult { namespace builders {

#define TEST_CLASS NetworkConfigBuilderTests

	namespace {
		using RegularTraits = test::RegularTransactionTraits<model::NetworkConfigTransaction>;
		using EmbeddedTraits = test::EmbeddedTransactionTraits<model::EmbeddedNetworkConfigTransaction>;

		using RegularAbsoluteTraits = test::RegularTransactionTraits<model::NetworkConfigAbsoluteHeightTransaction>;
		using EmbeddedAbsoluteTraits = test::EmbeddedTransactionTraits<model::EmbeddedNetworkConfigAbsoluteHeightTransaction>;

		struct TransactionProperties {
			uint64_t ApplyHeight = 0;
			std::string BlockChainConfig;
			std::string SupportedEntityVersions;
		};

		template<typename TTransaction>
		void AssertTransactionProperties(const TransactionProperties& expectedProperties, const TTransaction& transaction) {
			if constexpr(std::is_same_v<model::transaction_base_type_t<TTransaction>, model::NetworkConfigTransaction>)
				EXPECT_EQ(BlockDuration(expectedProperties.ApplyHeight), transaction.ApplyHeightDelta);
			else
				EXPECT_EQ(Height(expectedProperties.ApplyHeight), transaction.ApplyHeight);
			ASSERT_EQ(expectedProperties.BlockChainConfig.size(), transaction.BlockChainConfigSize);
			EXPECT_EQ_MEMORY(expectedProperties.BlockChainConfig.data(), transaction.BlockChainConfigPtr(), expectedProperties.BlockChainConfig.size());

			ASSERT_EQ(expectedProperties.SupportedEntityVersions.size(), transaction.SupportedEntityVersionsSize);
			EXPECT_EQ_MEMORY(expectedProperties.SupportedEntityVersions.data(), transaction.SupportedEntityVersionsPtr(), expectedProperties.SupportedEntityVersions.size());
		}

		template<typename TTraits>
		void AssertCanBuildTransaction(
			const TransactionProperties& expectedProperties,
			const consumer<NetworkConfigBuilder<model::transaction_base_type_t<typename TTraits::Transaction>>&>& buildTransaction) {
			// Arrange:
			auto networkId = static_cast<model::NetworkIdentifier>(0x62);
			auto signer = test::GenerateRandomByteArray<Key>();

			// Act:
			NetworkConfigBuilder<model::transaction_base_type_t<typename TTraits::Transaction>> builder(networkId, signer);
			buildTransaction(builder);
			auto pTransaction = TTraits::InvokeBuilder(builder);

			// Assert:
			TTraits::CheckFields(expectedProperties.BlockChainConfig.size() + expectedProperties.SupportedEntityVersions.size(), *pTransaction);
			EXPECT_EQ(signer, pTransaction->Signer);
			EXPECT_EQ(0x62000002, pTransaction->Version);
			EXPECT_EQ(model::transaction_base_type_t<typename TTraits::Transaction>::Entity_Type, pTransaction->Type);

			AssertTransactionProperties(expectedProperties, *pTransaction);
		}

		void WriteDataToFile(const std::string& fileName, const std::string& data) {
			io::RawFile file(fileName, io::OpenMode::Read_Write);
			file.write(RawBuffer{reinterpret_cast<const uint8_t*>(data.data()), data.size() });
			EXPECT_EQ(data.size(), file.size());
		}
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Regular_Delta) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<RegularTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Embedded_Delta) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<EmbeddedTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Regular_Absolute) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<RegularTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Embedded_Absolute) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<EmbeddedTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	// region constructor

	TRAITS_BASED_TEST(CanCreateTransactionWithDefaultValues) {
		// Assert:
		AssertCanBuildTransaction<TTraits>(TransactionProperties(), [](const auto&) {});
	}

	// endregion

	// region additional transaction fields

	TRAITS_BASED_TEST(CanSetApplyHeightDelta) {
		// Arrange:
		auto expectedProperties = TransactionProperties();
		expectedProperties.ApplyHeight = 10;

		// Assert:
		if constexpr(std::is_same_v<model::transaction_base_type_t<typename TTraits::Transaction>, model::NetworkConfigTransaction>)
			AssertCanBuildTransaction<TTraits>(expectedProperties, [](auto& builder) {
					builder.setApplyHeightDelta(BlockDuration{10});
			});
		else
			AssertCanBuildTransaction<TTraits>(expectedProperties, [](auto& builder) {
				builder.setApplyHeight(Height{10});
			});
	}

	TRAITS_BASED_TEST(CanSetRawBlockChainConfig) {
		// Arrange:
		auto expectedProperties = TransactionProperties();
		expectedProperties.BlockChainConfig = "aaa";

		// Assert:
		AssertCanBuildTransaction<TTraits>(expectedProperties, [](auto& builder) {
			std::string networkConfig("aaa");
			builder.setBlockChainConfig(RawBuffer{ reinterpret_cast<uint8_t*>(networkConfig.data()), networkConfig.size() });
		});
	}

	TRAITS_BASED_TEST(CanSetBlockChainConfigFromFile) {
		// Arrange:
		auto expectedProperties = TransactionProperties();
		expectedProperties.BlockChainConfig = "bbb";
		test::TempFileGuard tempFile("config-network.properties");
		WriteDataToFile(tempFile.name(), expectedProperties.BlockChainConfig);

		// Assert:
		AssertCanBuildTransaction<TTraits>(expectedProperties, [&tempFile](auto& builder) {
			builder.setBlockChainConfig(tempFile.name());
		});
	}

	TRAITS_BASED_TEST(CanSetRawSupportedEntityVersions) {
		// Arrange:
		auto expectedProperties = TransactionProperties();
		expectedProperties.SupportedEntityVersions = "ccc";

		// Assert:
		AssertCanBuildTransaction<TTraits>(expectedProperties, [](auto& builder) {
			std::string supportedEntityVersions("ccc");
			builder.setSupportedVersionsConfig(RawBuffer{ reinterpret_cast<uint8_t*>(supportedEntityVersions.data()), supportedEntityVersions.size() });
		});
	}

	TRAITS_BASED_TEST(CanSetSupportedEntityVersionsFromFile) {
		// Arrange:
		auto expectedProperties = TransactionProperties();
		expectedProperties.SupportedEntityVersions = "ddd";
		test::TempFileGuard tempFile("supported-entities.json");
		WriteDataToFile(tempFile.name(), expectedProperties.SupportedEntityVersions);

		// Assert:
		AssertCanBuildTransaction<TTraits>(expectedProperties, [&tempFile](auto& builder) {
			builder.setSupportedVersionsConfig(tempFile.name());
		});
	}

	// endregion
}}
