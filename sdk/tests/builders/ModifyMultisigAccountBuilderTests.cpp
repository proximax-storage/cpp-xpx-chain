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

#include "src/builders/ModifyMultisigAccountBuilder.h"
#include "sdk/tests/builders/test/BuilderTestUtils.h"

namespace catapult { namespace builders {

#define TEST_CLASS ModifyMultisigAccountBuilderTests

	namespace {
		using MultisigRegularTraits = test::RegularTransactionTraits<model::ModifyMultisigAccountTransaction>;
		using MultisigEmbeddedTraits = test::EmbeddedTransactionTraits<model::EmbeddedModifyMultisigAccountTransaction>;
		using ReputationRegularTraits = test::RegularTransactionTraits<model::ModifyMultisigAccountAndReputationTransaction>;
		using ReputationEmbeddedTraits = test::EmbeddedTransactionTraits<model::EmbeddedModifyMultisigAccountAndReputationTransaction>;
		using Modifications = std::vector<model::CosignatoryModification>;
		using MultisigBuilder = ModifyMultisigAccountBuilder;
		using ReputationBuilder = ModifyMultisigAccountAndReputationBuilder;

		constexpr auto MultisigType = model::Entity_Type_Modify_Multisig_Account;
		constexpr auto ReputationType = model::Entity_Type_Modify_Multisig_Account_And_Reputation;

		struct TransactionProperties {
		public:
			TransactionProperties()
					: MinRemovalDelta(0)
					, MinApprovalDelta(0)
			{}

		public:
			int8_t MinRemovalDelta;
			int8_t MinApprovalDelta;
			std::vector<model::CosignatoryModification> Modifications;
		};

		template<typename TTransaction>
		void AssertTransactionProperties(const TransactionProperties& expectedProperties, const TTransaction& transaction) {
			EXPECT_EQ(expectedProperties.MinRemovalDelta, transaction.MinRemovalDelta);
			EXPECT_EQ(expectedProperties.MinApprovalDelta, transaction.MinApprovalDelta);

			auto i = 0u;
			const auto* pModification = transaction.ModificationsPtr();
			for (const auto& modification : expectedProperties.Modifications) {
				auto rawType = static_cast<uint16_t>(modification.ModificationType);
				EXPECT_EQ(modification.ModificationType, pModification->ModificationType) << "type " << rawType << " at index " << i;
				EXPECT_EQ(modification.CosignatoryPublicKey, pModification->CosignatoryPublicKey) << "at index " << i;
				++pModification;
			}
		}

		template<typename TTraits, typename TBuilder, model::EntityType TransactionType>
		void AssertCanBuildTransaction(
				size_t additionalSize,
				const TransactionProperties& expectedProperties,
				const consumer<TBuilder&>& buildTransaction) {
			// Arrange:
			auto networkId = static_cast<model::NetworkIdentifier>(0x62);
			auto signer = test::GenerateRandomData<Key_Size>();

			// Act:
			TBuilder builder(networkId, signer);
			buildTransaction(builder);
			auto pTransaction = TTraits::InvokeBuilder(builder);

			// Assert:
			TTraits::CheckFields(additionalSize, *pTransaction);
			EXPECT_EQ(signer, pTransaction->Signer);
			EXPECT_EQ(0x6203, pTransaction->Version);
			EXPECT_EQ(TransactionType, pTransaction->Type);

			AssertTransactionProperties(expectedProperties, *pTransaction);
		}
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits, typename TBuilder, model::EntityType TransactionType> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_MultisigRegular) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<MultisigRegularTraits, MultisigBuilder, MultisigType>(); } \
	TEST(TEST_CLASS, TEST_NAME##_MultisigEmbedded) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<MultisigEmbeddedTraits, MultisigBuilder, MultisigType>(); } \
	TEST(TEST_CLASS, TEST_NAME##_ReputationRegular) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ReputationRegularTraits, ReputationBuilder, ReputationType>(); } \
	TEST(TEST_CLASS, TEST_NAME##_ReputationEmbedded) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ReputationEmbeddedTraits, ReputationBuilder, ReputationType>(); } \
	template<typename TTraits, typename TBuilder, model::EntityType TransactionType> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	// region constructor

	TRAITS_BASED_TEST(CanCreateTransactionWithDefaultValues) {
		// Assert:
		AssertCanBuildTransaction<TTraits, TBuilder, TransactionType>(0, TransactionProperties(), [](auto&) {});
	}

	// endregion

	// region min cosignatory settings

	TRAITS_BASED_TEST(CanSetMinRemovalDelta) {
		// Arrange:
		auto expectedProperties = TransactionProperties();
		expectedProperties.MinRemovalDelta = 3;

		// Assert:
		AssertCanBuildTransaction<TTraits, TBuilder, TransactionType>(0, expectedProperties, [](auto& builder) {
			builder.setMinRemovalDelta(3);
		});
	}

	TRAITS_BASED_TEST(CanSetMinApprovalDelta) {
		// Arrange:
		auto expectedProperties = TransactionProperties();
		expectedProperties.MinApprovalDelta = 3;

		// Assert:
		AssertCanBuildTransaction<TTraits, TBuilder, TransactionType>(0, expectedProperties, [](auto& builder) {
			builder.setMinApprovalDelta(3);
		});
	}

	// endregion

	// region modifications

	namespace {
		auto CreateModifications(uint8_t count) {
			Modifications modifications;
			for (auto i = 0u; i < count; ++i) {
				auto type = 0 == i % 2 ? model::CosignatoryModificationType::Add : model::CosignatoryModificationType::Del;
				modifications.push_back(model::CosignatoryModification{ type, test::GenerateRandomData<Key_Size>() });
			}

			return modifications;
		}

		template<typename TBuilder>
		void AddAll(TBuilder& builder, const Modifications& modifications) {
			for (const auto& modification : modifications)
				builder.addCosignatoryModification(modification.ModificationType, modification.CosignatoryPublicKey);
		}
	}

	TRAITS_BASED_TEST(CanAddSingleModification) {
		// Arrange:
		auto expectedProperties = TransactionProperties();
		expectedProperties.Modifications = CreateModifications(1);
		const auto& modifications = expectedProperties.Modifications;
		auto totalCosignatorySize = sizeof(model::CosignatoryModification);

		// Assert:
		AssertCanBuildTransaction<TTraits, TBuilder, TransactionType>(totalCosignatorySize, expectedProperties, [&modifications](auto& builder) {
			AddAll(builder, modifications);
		});
	}

	TRAITS_BASED_TEST(CanAddMultipleModifications) {
		// Arrange:
		auto expectedProperties = TransactionProperties();
		expectedProperties.Modifications = CreateModifications(5);
		const auto& modifications = expectedProperties.Modifications;
		auto totalCosignatorySize = 5 * sizeof(model::CosignatoryModification);

		// Assert:
		AssertCanBuildTransaction<TTraits, TBuilder, TransactionType>(totalCosignatorySize, expectedProperties, [&modifications](auto& builder) {
			AddAll(builder, modifications);
		});
	}

	TRAITS_BASED_TEST(CanSetDeltasAndAddModifications) {
		// Arrange:
		auto expectedProperties = TransactionProperties();
		expectedProperties.MinRemovalDelta = -3;
		expectedProperties.MinApprovalDelta = 3;
		expectedProperties.Modifications = CreateModifications(4);
		const auto& modifications = expectedProperties.Modifications;
		auto totalCosignatorySize = 4 * sizeof(model::CosignatoryModification);

		// Assert:
		AssertCanBuildTransaction<TTraits, TBuilder, TransactionType>(totalCosignatorySize, expectedProperties, [&modifications](auto& builder) {
			builder.setMinRemovalDelta(-3);
			builder.setMinApprovalDelta(3);
			AddAll(builder, modifications);
		});
	}

	// endregion
}}
