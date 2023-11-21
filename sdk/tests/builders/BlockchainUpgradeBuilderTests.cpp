/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/builders/BlockchainUpgradeBuilder.h"
#include "tests/builders/test/BuilderTestUtils.h"

namespace catapult { namespace builders {

#define TEST_CLASS BlockchainUpgradeBuilderTests

	namespace {
		using RegularTraits = test::RegularTransactionTraits<model::BlockchainUpgradeTransaction>;
		using EmbeddedTraits = test::EmbeddedTransactionTraits<model::EmbeddedBlockchainUpgradeTransaction>;

		struct TransactionProperties {
			BlockDuration UpgradePeriod;
			BlockchainVersion NewBlockchainVersion;
		};

		template<typename TTransaction>
		void AssertTransactionProperties(const TransactionProperties& expectedProperties, const TTransaction& transaction) {
			EXPECT_EQ(expectedProperties.UpgradePeriod, transaction.UpgradePeriod);
			EXPECT_EQ(expectedProperties.NewBlockchainVersion, transaction.NewBlockchainVersion);
		}

		template<typename TTraits>
		void AssertCanBuildTransaction(
			const TransactionProperties& expectedProperties,
			const consumer<BlockchainUpgradeBuilder&>& buildTransaction) {
			// Arrange:
			auto networkId = static_cast<model::NetworkIdentifier>(0x62);
			auto signer = test::GenerateRandomByteArray<Key>();

			// Act:
			BlockchainUpgradeBuilder builder(networkId, signer);
			buildTransaction(builder);
			auto pTransaction = TTraits::InvokeBuilder(builder);

			// Assert:
			TTraits::CheckFields(0u, *pTransaction);
			EXPECT_EQ(signer, pTransaction->Signer);
			EXPECT_EQ(0x62000001, pTransaction->Version);
			EXPECT_EQ(model::Entity_Type_Blockchain_Upgrade, pTransaction->Type);

			AssertTransactionProperties(expectedProperties, *pTransaction);
		}
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Regular) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<RegularTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Embedded) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<EmbeddedTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	// region constructor

	TRAITS_BASED_TEST(CanCreateTransactionWithDefaultValues) {
		// Assert:
		AssertCanBuildTransaction<TTraits>(TransactionProperties(), [](const auto&) {});
	}

	// endregion

	// region additional transaction fields

	TRAITS_BASED_TEST(CanSetUpgradePeriod) {
		// Arrange:
		auto expectedProperties = TransactionProperties();
		expectedProperties.UpgradePeriod = BlockDuration{10};

		// Assert:
		AssertCanBuildTransaction<TTraits>(expectedProperties, [](auto& builder) {
			builder.setUpgradePeriod(BlockDuration{10});
		});
	}

	TRAITS_BASED_TEST(CanSetNewBlockchainVersion) {
		// Arrange:
		auto expectedProperties = TransactionProperties();
		expectedProperties.NewBlockchainVersion = BlockchainVersion{7};

		// Assert:
		AssertCanBuildTransaction<TTraits>(expectedProperties, [](auto& builder) {
			builder.setNewBlockchainVersion(BlockchainVersion{7});
		});
	}

	// endregion
}}
