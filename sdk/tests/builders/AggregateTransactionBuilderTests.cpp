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

#include "catapult/crypto/Signature.h"
#include "src/builders/AggregateTransactionBuilder.h"
#include "catapult/model/EntityHasher.h"
#include "sdk/tests/builders/test/BuilderTestUtils.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/EntityTestUtils.h"
#include "tests/test/core/mocks/MockTransaction.h"

namespace catapult { namespace builders {

#define TEST_CLASS AggregateTransactionBuilderTests

	namespace {

		template<typename TDescriptor>
		struct RegularTraits  : public test::RegularTransactionTraits<model::AggregateTransaction<TDescriptor>> {
			using Descriptor = TDescriptor;
			static constexpr model::EntityType BondedType = TDescriptor::BondedType;
			static constexpr model::EntityType CompleteType = TDescriptor::CompleteType;
		};

		template<typename TTraits>
		class TestContext {
		public:
			explicit TestContext(size_t numTransactions)
					: m_networkId(static_cast<model::NetworkIdentifier>(0x62))
					, m_signer(test::GenerateRandomByteArray<Key>()) {
				for (auto i = 0u; i < numTransactions; ++i)
					m_pTransactions.push_back(mocks::CreateEmbeddedMockTransaction(static_cast<uint16_t>(31 + i)));
			}

		public:
			auto buildTransaction() {
				AggregateTransactionBuilder<typename TTraits::Descriptor> builder(m_networkId, m_signer);
				for (const auto& pTransaction : m_pTransactions)
					builder.addTransaction(test::CopyEntity(*pTransaction));

				return builder.build();
			}

			void assertTransaction(const model::AggregateTransaction<typename TTraits::Descriptor>& transaction, size_t numCosignatures, model::EntityType type) {
				auto numTransactions = m_pTransactions.size();
				size_t additionalSize = numTransactions * sizeof(mocks::EmbeddedMockTransaction);
				for (auto i = 0u; i < numTransactions; ++i)
					additionalSize += 31 + i;

				additionalSize += numCosignatures * sizeof(typename TTraits::Descriptor::CosignatureType);
				TTraits::CheckFields(additionalSize, transaction);
				EXPECT_EQ(m_signer, transaction.Signer);
				if constexpr(std::is_same_v<model::AggregateTransactionRawDescriptor, typename TTraits::Descriptor>)
					EXPECT_EQ(0x62000003, transaction.Version);
				else
					EXPECT_EQ(0x62000001, transaction.Version);
				EXPECT_EQ(type, transaction.Type);

				auto i = 0u;
				for (const auto& embedded : transaction.Transactions()) {
					ASSERT_EQ(m_pTransactions[i]->Size, embedded.Size) << "invalid size of transaction " << i;
					EXPECT_EQ_MEMORY(m_pTransactions[i].get(), &embedded, embedded.Size) << "invalid transaction " << i;
					++i;
				}
			}

		private:
			const model::NetworkIdentifier m_networkId;
			const Key m_signer;
			std::vector<model::UniqueEntityPtr<mocks::EmbeddedMockTransaction>> m_pTransactions;
		};

		template<typename TTraits>
		void AssertAggregateBuilderTransaction(size_t numTransactions) {
			// Arrange:
			TestContext<TTraits> context(numTransactions);

			// Act:
			auto pTransaction = context.buildTransaction();

			// Assert:
			context.assertTransaction(*pTransaction, 0, TTraits::BondedType);
		}

		auto GenerateKeys(size_t numKeysV1, size_t numKeysV2) {
			std::vector<crypto::KeyPair> keys;
			for (auto i = 0u; i < numKeysV1; ++i)
				keys.push_back(test::GenerateKeyPair(1));
			for (auto i = 0u; i < numKeysV2; ++i)
				keys.push_back(test::GenerateKeyPair(2));
			return keys;
		}

		template<typename TDescriptor>
		RawBuffer TransactionDataBuffer(const model::AggregateTransaction<TDescriptor>& transaction) {
			return {
				reinterpret_cast<const uint8_t*>(&transaction) + model::VerifiableEntity::Header_Size,
				sizeof(model::AggregateTransaction<TDescriptor>) - model::VerifiableEntity::Header_Size + transaction.PayloadSize
			};
		}

		template<typename TTraits>
		void AssertAggregateCosignaturesTransaction(size_t numCosignatures) {
			// Arrange: create transaction with 3 embedded transactions
			TestContext<TTraits> context(3);
			auto generationHash = test::GenerateRandomByteArray<GenerationHash>();
			AggregateCosignatureAppender builder(generationHash, context.buildTransaction());
			auto cosigners = GenerateKeys(numCosignatures/2+numCosignatures%2, !std::is_same_v<typename TTraits::Descriptor, model::AggregateTransactionExtendedDescriptor> ? 0 : numCosignatures/2);

			// Act:
			for (const auto& cosigner : cosigners)
				builder.cosign(cosigner);

			auto pTransaction = builder.build();

			// Assert:
			context.assertTransaction(*pTransaction, cosigners.size(), TTraits::CompleteType);
			auto hash = model::CalculateHash(*pTransaction, generationHash, TransactionDataBuffer(*pTransaction));
			const auto* pCosignature = pTransaction->CosignaturesPtr();
			for (const auto& cosigner : cosigners) {
				EXPECT_EQ(cosigner.publicKey(), pCosignature->Signer) << "invalid signer";
				EXPECT_TRUE(crypto::SignatureFeatureSolver::Verify(pCosignature->Signer, {hash}, pCosignature->GetRawSignature(), pCosignature->GetDerivationScheme()))
						<< "invalid cosignature " << pCosignature->Signature;
				++pCosignature;
			}
		}
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
    template<typename TTraits>                                 \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<RegularTraits<model::AggregateTransactionRawDescriptor>>(); } \
	TEST(TEST_CLASS, TEST_NAME##_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<RegularTraits<model::AggregateTransactionExtendedDescriptor>>(); } \
    template<typename TTraits> \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TRAITS_BASED_TEST(AggregateBuilderCreatesProperTransaction_SingleTransaction) {
		// Assert:
		AssertAggregateBuilderTransaction<TTraits>(1);
	}

	TRAITS_BASED_TEST(AggregateBuilderCreatesProperTransaction_MultipleTransactions) {
		// Assert:
		AssertAggregateBuilderTransaction<TTraits>(3);
	}

	TRAITS_BASED_TEST(AggregateCosignatureAppenderAddsProperSignature_SingleCosignatory) {
		// Assert:
		AssertAggregateCosignaturesTransaction<TTraits>(1);
	}

	TRAITS_BASED_TEST(AggregateCosignatureAppenderAddsProperSignature_MultipleCosignatories) {
		// Assert:
		AssertAggregateCosignaturesTransaction<TTraits>(3);
	}
#undef TRAITS_BASED_TEST
}}
