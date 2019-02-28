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

#include "src/builders/AggregateTransactionBuilder.h"
#include "catapult/crypto/Signer.h"
#include "catapult/model/EntityHasher.h"
#include "sdk/tests/builders/test/BuilderTestUtils.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/EntityTestUtils.h"
#include "tests/test/core/mocks/MockTransaction.h"

namespace catapult { namespace builders {

#define TEST_CLASS AggregateTransactionBuilderTests

	namespace {
		using RegularTraits = test::RegularTransactionTraits<model::AggregateTransaction>;

		class TestContext {
		public:
			explicit TestContext(size_t numTransactions)
					: m_networkId(static_cast<model::NetworkIdentifier>(0x62))
					, m_signer(test::GenerateRandomData<Key_Size>()) {
				for (auto i = 0u; i < numTransactions; ++i)
					m_pTransactions.push_back(mocks::CreateEmbeddedMockTransaction(static_cast<uint16_t>(31 + i)));
			}

		public:
			auto buildTransaction() {
				AggregateTransactionBuilder builder(m_networkId, m_signer);
				for (const auto& pTransaction : m_pTransactions)
					builder.addTransaction(test::CopyEntity(*pTransaction));

				return builder.build();
			}

			void assertTransaction(const model::AggregateTransaction& transaction, size_t numCosignatures, model::EntityType type) {
				auto numTransactions = m_pTransactions.size();
				size_t additionalSize = numTransactions * sizeof(mocks::EmbeddedMockTransaction);
				for (auto i = 0u; i < numTransactions; ++i)
					additionalSize += 31 + i;

				additionalSize += numCosignatures * sizeof(model::Cosignature);
				RegularTraits::CheckFields(additionalSize, transaction);
				EXPECT_EQ(m_signer, transaction.Signer);
				EXPECT_EQ(0x6202, transaction.Version);
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
			std::vector<std::unique_ptr<mocks::EmbeddedMockTransaction>> m_pTransactions;
		};

		void AssertAggregateBuilderTransaction(size_t numTransactions) {
			// Arrange:
			TestContext context(numTransactions);

			// Act:
			auto pTransaction = context.buildTransaction();

			// Assert:
			context.assertTransaction(*pTransaction, 0, model::Entity_Type_Aggregate_Bonded);
		}

		auto GenerateKeys(size_t numKeys) {
			std::vector<crypto::KeyPair> keys;
			for (auto i = 0u; i < numKeys; ++i)
				keys.push_back(test::GenerateKeyPair());

			return keys;
		}

		RawBuffer TransactionDataBuffer(const model::AggregateTransaction& transaction) {
			return {
				reinterpret_cast<const uint8_t*>(&transaction) + model::VerifiableEntity::Header_Size,
				sizeof(model::AggregateTransaction) - model::VerifiableEntity::Header_Size + transaction.PayloadSize
			};
		}

		void AssertAggregateCosignaturesTransaction(size_t numCosignatures) {
			// Arrange: create transaction with 3 embedded transactions
			TestContext context(3);
			AggregateCosignatureAppender builder(context.buildTransaction());
			auto cosigners = GenerateKeys(numCosignatures);

			// Act:
			for (const auto& cosigner : cosigners)
				builder.cosign(cosigner);

			auto pTransaction = builder.build();

			// Assert:
			context.assertTransaction(*pTransaction, cosigners.size(), model::Entity_Type_Aggregate_Complete);
			auto hash = model::CalculateHash(*pTransaction, TransactionDataBuffer(*pTransaction));
			const auto* pCosignature = pTransaction->CosignaturesPtr();
			for (const auto& cosigner : cosigners) {
				EXPECT_EQ(cosigner.publicKey(), pCosignature->Signer) << "invalid signer";
				EXPECT_TRUE(crypto::Verify(pCosignature->Signer, hash, pCosignature->Signature))
						<< "invalid cosignature " << utils::HexFormat(pCosignature->Signature);
				++pCosignature;
			}
		}
	}

	TEST(TEST_CLASS, AggregateBuilderCreatesProperTransaction_SingleTransaction) {
		// Assert:
		AssertAggregateBuilderTransaction(1);
	}

	TEST(TEST_CLASS, AggregateBuilderCreatesProperTransaction_MultipleTransactions) {
		// Assert:
		AssertAggregateBuilderTransaction(3);
	}

	TEST(TEST_CLASS, AggregateCosignatureAppenderAddsProperSignature_SingleCosignatory) {
		// Assert:
		AssertAggregateCosignaturesTransaction(1);
	}

	TEST(TEST_CLASS, AggregateCosignatureAppenderAddsProperSignature_MultipleCosignatories) {
		// Assert:
		AssertAggregateCosignaturesTransaction(3);
	}
}}
