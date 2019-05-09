/**
*** Copyright (c) 2018-present,
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

#include "src/ModifyContractMapper.h"
#include "sdk/src/builders/ModifyContractBuilder.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/constants.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS ModifyContractMapperTests

	namespace {
		DEFINE_MONGO_TRANSACTION_PLUGIN_TEST_TRAITS(ModifyContract)

		using ModificationType = model::CosignatoryModificationType;

		auto CreateModifyContractTransactionBuilder(
				const Key& signer,
				std::initializer_list<model::CosignatoryModification> customers,
				std::initializer_list<model::CosignatoryModification> executors,
				std::initializer_list<model::CosignatoryModification> verifiers) {
			builders::ModifyContractBuilder builder(model::NetworkIdentifier::Mijin_Test, signer);
			builder.setDurationDelta(100);
			builder.setHash(test::GenerateRandomData<Hash256_Size>());
			for (const auto& modification : customers)
				builder.addCustomerModification(modification.ModificationType, modification.CosignatoryPublicKey);
			for (const auto& modification : executors)
				builder.addExecutorModification(modification.ModificationType, modification.CosignatoryPublicKey);
			for (const auto& modification : verifiers)
				builder.addVerifierModification(modification.ModificationType, modification.CosignatoryPublicKey);

			return builder;
		}

		void AssertEqualModifications(
				const bsoncxx::array::view& dbModifications,
				uint8_t modificationsCount,
				const model::CosignatoryModification* pModification) {
			ASSERT_EQ(modificationsCount, std::distance(dbModifications.cbegin(), dbModifications.cend()));
			auto iter = dbModifications.cbegin();
			for (auto i = 0u; i < modificationsCount; ++i, ++pModification, ++iter) {
				EXPECT_EQ(
						pModification->ModificationType,
						static_cast<ModificationType>(test::GetUint32(iter->get_document().view(), "type")));
				EXPECT_EQ(pModification->CosignatoryPublicKey, test::GetKeyValue(iter->get_document().view(), "cosignatoryPublicKey"));
			}
		}

		template<typename TTransaction>
		void AssertEqualNonInheritedTransferData(const TTransaction& transaction, const bsoncxx::document::view& dbTransaction) {
			EXPECT_EQ(transaction.DurationDelta, test::GetInt64(dbTransaction, "durationDelta"));
			EXPECT_EQ(transaction.Hash, test::GetHashValue(dbTransaction, "hash"));

			AssertEqualModifications(dbTransaction["customers"].get_array().value,
					transaction.CustomerModificationCount, transaction.CustomerModificationsPtr());
			AssertEqualModifications(dbTransaction["executors"].get_array().value,
					transaction.ExecutorModificationCount, transaction.ExecutorModificationsPtr());
			AssertEqualModifications(dbTransaction["verifiers"].get_array().value,
					transaction.VerifierModificationCount, transaction.VerifierModificationsPtr());
		}

		template<typename TTraits>
		void AssertCanMapModifyContractTransaction(
				std::initializer_list<model::CosignatoryModification> customers,
				std::initializer_list<model::CosignatoryModification> executors,
				std::initializer_list<model::CosignatoryModification> verifiers) {
			// Arrange:
			auto signer = test::GenerateRandomData<Key_Size>();
			auto pBuilder = CreateModifyContractTransactionBuilder(signer, customers, executors, verifiers);
			auto pTransaction = TTraits::Adapt(pBuilder);
			auto pPlugin = TTraits::CreatePlugin();

			// Act:
			mappers::bson_stream::document builder;
			pPlugin->streamTransaction(builder, *pTransaction);
			auto view = builder.view();

			// Assert:
			EXPECT_EQ(5u, test::GetFieldCount(view));
			AssertEqualNonInheritedTransferData(*pTransaction, view);
		}
	}

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, model::Entity_Type_Modify_Contract)

	// region streamTransaction

	PLUGIN_TEST(CanMapModifyContractTransactionWithoutModifications) {
		// Assert:
		AssertCanMapModifyContractTransaction<TTraits>({}, {}, {});
	}

	PLUGIN_TEST(CanMapModifyContractTransactionWithSingleCustomerModification) {
		// Assert:
		AssertCanMapModifyContractTransaction<TTraits>({ { ModificationType::Add, test::GenerateRandomData<Key_Size>() } }, {}, {});
	}

	PLUGIN_TEST(CanMapModifyContractTransactionWithSingleExecutorModification) {
		// Assert:
		AssertCanMapModifyContractTransaction<TTraits>({}, { { ModificationType::Add, test::GenerateRandomData<Key_Size>() } }, {});
	}

	PLUGIN_TEST(CanMapModifyContractTransactionWithSingleVerifierModification) {
		// Assert:
		AssertCanMapModifyContractTransaction<TTraits>({}, {}, { { ModificationType::Add, test::GenerateRandomData<Key_Size>() } });
	}

	PLUGIN_TEST(CanMapModifyContractTransactionWithMultipleCustomerModifications) {
		// Assert:
		AssertCanMapModifyContractTransaction<TTraits>({
			{ ModificationType::Add, test::GenerateRandomData<Key_Size>() },
			{ ModificationType::Del, test::GenerateRandomData<Key_Size>() },
			{ ModificationType::Add, test::GenerateRandomData<Key_Size>() },
		}, {}, {});
	}

	PLUGIN_TEST(CanMapModifyContractTransactionWithMultipleExecutorModifications) {
		// Assert:
		AssertCanMapModifyContractTransaction<TTraits>({}, {
			{ ModificationType::Add, test::GenerateRandomData<Key_Size>() },
			{ ModificationType::Del, test::GenerateRandomData<Key_Size>() },
			{ ModificationType::Add, test::GenerateRandomData<Key_Size>() },
		}, {});
	}

	PLUGIN_TEST(CanMapModifyContractTransactionWithMultipleVerifierModifications) {
		// Assert:
		AssertCanMapModifyContractTransaction<TTraits>({}, {}, {
			{ ModificationType::Add, test::GenerateRandomData<Key_Size>() },
			{ ModificationType::Del, test::GenerateRandomData<Key_Size>() },
			{ ModificationType::Add, test::GenerateRandomData<Key_Size>() },
		});
	}

	PLUGIN_TEST(CanMapModifyContractTransactionWithMultipleModifications) {
		// Assert:
		AssertCanMapModifyContractTransaction<TTraits>({
			{ ModificationType::Add, test::GenerateRandomData<Key_Size>() },
			{ ModificationType::Del, test::GenerateRandomData<Key_Size>() },
			{ ModificationType::Add, test::GenerateRandomData<Key_Size>() },
		}, {
			{ ModificationType::Add, test::GenerateRandomData<Key_Size>() },
			{ ModificationType::Del, test::GenerateRandomData<Key_Size>() },
			{ ModificationType::Add, test::GenerateRandomData<Key_Size>() },
		}, {
			{ ModificationType::Add, test::GenerateRandomData<Key_Size>() },
			{ ModificationType::Del, test::GenerateRandomData<Key_Size>() },
			{ ModificationType::Add, test::GenerateRandomData<Key_Size>() },
		});
	}

	// endregion
}}}
