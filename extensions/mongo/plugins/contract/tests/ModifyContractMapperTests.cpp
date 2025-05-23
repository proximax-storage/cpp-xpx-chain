/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/ModifyContractMapper.h"
#include "sdk/src/builders/ModifyContractBuilder.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoTransactionPluginTestUtils.h"

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
			builder.setHash(test::GenerateRandomByteArray<Hash256>());
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
			auto signer = test::GenerateRandomByteArray<Key>();
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

	DEFINE_BASIC_MONGO_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS,,, model::Entity_Type_Modify_Contract)

	// region streamTransaction

	PLUGIN_TEST(CanMapModifyContractTransactionWithoutModifications) {
		// Assert:
		AssertCanMapModifyContractTransaction<TTraits>({}, {}, {});
	}

	PLUGIN_TEST(CanMapModifyContractTransactionWithSingleCustomerModification) {
		// Assert:
		AssertCanMapModifyContractTransaction<TTraits>({ { ModificationType::Add, test::GenerateRandomByteArray<Key>() } }, {}, {});
	}

	PLUGIN_TEST(CanMapModifyContractTransactionWithSingleExecutorModification) {
		// Assert:
		AssertCanMapModifyContractTransaction<TTraits>({}, { { ModificationType::Add, test::GenerateRandomByteArray<Key>() } }, {});
	}

	PLUGIN_TEST(CanMapModifyContractTransactionWithSingleVerifierModification) {
		// Assert:
		AssertCanMapModifyContractTransaction<TTraits>({}, {}, { { ModificationType::Add, test::GenerateRandomByteArray<Key>() } });
	}

	PLUGIN_TEST(CanMapModifyContractTransactionWithMultipleCustomerModifications) {
		// Assert:
		AssertCanMapModifyContractTransaction<TTraits>({
			{ ModificationType::Add, test::GenerateRandomByteArray<Key>() },
			{ ModificationType::Del, test::GenerateRandomByteArray<Key>() },
			{ ModificationType::Add, test::GenerateRandomByteArray<Key>() },
		}, {}, {});
	}

	PLUGIN_TEST(CanMapModifyContractTransactionWithMultipleExecutorModifications) {
		// Assert:
		AssertCanMapModifyContractTransaction<TTraits>({}, {
			{ ModificationType::Add, test::GenerateRandomByteArray<Key>() },
			{ ModificationType::Del, test::GenerateRandomByteArray<Key>() },
			{ ModificationType::Add, test::GenerateRandomByteArray<Key>() },
		}, {});
	}

	PLUGIN_TEST(CanMapModifyContractTransactionWithMultipleVerifierModifications) {
		// Assert:
		AssertCanMapModifyContractTransaction<TTraits>({}, {}, {
			{ ModificationType::Add, test::GenerateRandomByteArray<Key>() },
			{ ModificationType::Del, test::GenerateRandomByteArray<Key>() },
			{ ModificationType::Add, test::GenerateRandomByteArray<Key>() },
		});
	}

	PLUGIN_TEST(CanMapModifyContractTransactionWithMultipleModifications) {
		// Assert:
		AssertCanMapModifyContractTransaction<TTraits>({
			{ ModificationType::Add, test::GenerateRandomByteArray<Key>() },
			{ ModificationType::Del, test::GenerateRandomByteArray<Key>() },
			{ ModificationType::Add, test::GenerateRandomByteArray<Key>() },
		}, {
			{ ModificationType::Add, test::GenerateRandomByteArray<Key>() },
			{ ModificationType::Del, test::GenerateRandomByteArray<Key>() },
			{ ModificationType::Add, test::GenerateRandomByteArray<Key>() },
		}, {
			{ ModificationType::Add, test::GenerateRandomByteArray<Key>() },
			{ ModificationType::Del, test::GenerateRandomByteArray<Key>() },
			{ ModificationType::Add, test::GenerateRandomByteArray<Key>() },
		});
	}

	// endregion
}}}
