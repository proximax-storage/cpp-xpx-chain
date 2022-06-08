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

#include "partialtransaction/src/PtUtils.h"
#include "partialtransaction/tests/test/AggregateTransactionTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace partialtransaction {

#define TEST_CLASS PtUtilsTests

	// region StitchAggregate

	namespace {
		template<typename TDescriptor>
		void AssertCanStitchAggregate(size_t numCosignatures) {
			// Arrange:
			auto pTransaction = test::CreateAggregateTransaction<TDescriptor>(1).pTransaction;
			auto cosignatures = test::GenerateRandomDataVector<model::CosignatureInfo>(numCosignatures);

			// Act:
			auto pStitchedTransaction = StitchAggregate({ pTransaction.get(), &cosignatures });

			// Assert:
			test::AssertStitchedTransaction(*pStitchedTransaction, *pTransaction, cosignatures, 0);

			// Sanity:
			EXPECT_EQ(numCosignatures, static_cast<const model::AggregateTransaction<TDescriptor>&>(*pStitchedTransaction).CosignaturesCount());
		}
	}

	TEST(TEST_CLASS, StitchAggregateV1CanStitchTransactionWithoutCosignatures) {
		// Assert:
		AssertCanStitchAggregate<model::AggregateTransactionRawDescriptor>(0);
	}

	TEST(TEST_CLASS, StitchAggregateV2CanStitchTransactionWithoutCosignatures) {
		// Assert:
		AssertCanStitchAggregate<model::AggregateTransactionExtendedDescriptor>(0);
	}

	TEST(TEST_CLASS, StitchAggregateV1CanStitchTransactionWithCosignatures) {
		// Assert:
			AssertCanStitchAggregate<model::AggregateTransactionRawDescriptor>(3);
	}

	TEST(TEST_CLASS, StitchAggregateV2CanStitchTransactionWithCosignatures) {
		// Assert:
		AssertCanStitchAggregate<model::AggregateTransactionExtendedDescriptor>(3);
	}

	// endregion

	// region SplitCosignedTransactionInfos

	namespace {
		struct SplitForwardingCapture {
			std::vector<model::TransactionRange> TransactionRanges;
			std::vector<model::DetachedCosignature> Cosignatures;
		};

		SplitForwardingCapture SplitAndCapture(const CosignedTransactionInfos& transactionInfos) {
			SplitForwardingCapture capture;
			SplitCosignedTransactionInfos(
					CosignedTransactionInfos(transactionInfos), // make a copy (because parameter is rvalue)
					[&capture](auto&& range) {
						capture.TransactionRanges.push_back(std::move(range));
					},
					[&capture](auto&& cosignature) {
						capture.Cosignatures.push_back(std::move(cosignature));
					});
			return capture;
		}

		template<typename TDescriptor>
		typename TDescriptor::CosignatureType GenerateRandomCosignature();
		template<>
		model::Cosignature<SignatureLayout::Raw> GenerateRandomCosignature<model::AggregateTransactionRawDescriptor>(){
			return { test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<RawSignature>() };
		}
		template<>
		model::Cosignature<SignatureLayout::Extended> GenerateRandomCosignature<model::AggregateTransactionExtendedDescriptor>(){
			return { test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Signature>() };
		}


		class TransactionGenerator {
		public:
			template<typename TDescriptor>
			void addData(CosignedTransactionInfos& transactionInfos, size_t numCosignatures) {
				model::CosignedTransactionInfo transactionInfo;
				transactionInfo.pTransaction = test::CreateAggregateTransaction<TDescriptor>(1).pTransaction;
				for (auto i = 0u; i < numCosignatures; ++i)
					transactionInfo.Cosignatures.push_back(GenerateRandomCosignature<TDescriptor>().ToInfo());

				transactionInfos.push_back(transactionInfo);
				m_transactionInfos.push_back(transactionInfo);
			}

			void assertData(const model::TransactionRange& transactionRange) {
				ASSERT_EQ(m_transactionInfos.size(), transactionRange.size());

				auto i = 0u;
				for (const auto& transaction : transactionRange) {
					// notice that stitched transaction is always passed to range
					if(IsAggregateV1(m_transactionInfos[i].pTransaction->Type))
					{
						const auto& originalTransaction =
								static_cast<const model::AggregateTransaction<model::AggregateTransactionRawDescriptor>&>(*m_transactionInfos[i].pTransaction);
						test::AssertStitchedTransaction<model::AggregateTransactionRawDescriptor>(transaction, originalTransaction, m_transactionInfos[i].Cosignatures, 0);
					}
					else
					{
						const auto& originalTransaction =
								static_cast<const model::AggregateTransaction<model::AggregateTransactionExtendedDescriptor>&>(*m_transactionInfos[i].pTransaction);
						test::AssertStitchedTransaction<model::AggregateTransactionExtendedDescriptor>(transaction, originalTransaction, m_transactionInfos[i].Cosignatures, 0);
					}

					++i;
				}
			}

		private:
			partialtransaction::CosignedTransactionInfos m_transactionInfos;
		};

		class CosignatureGenerator {
		public:
			template<typename TDescriptor>
			void addData(CosignedTransactionInfos& transactionInfos, size_t numCosignatures) {
				model::CosignedTransactionInfo transactionInfo;
				transactionInfo.EntityHash = test::GenerateRandomByteArray<Hash256>();
				for (auto i = 0u; i < numCosignatures; ++i) {
					auto cosignature = GenerateRandomCosignature<TDescriptor>();
					transactionInfo.Cosignatures.push_back(cosignature.ToInfo());
					m_cosignatures.push_back(
							model::DetachedCosignature(cosignature.Signer, cosignature.GetRawSignature(), cosignature.GetDerivationScheme(), transactionInfo.EntityHash));
				}

				transactionInfos.push_back(transactionInfo);
			}

			void assertData(const std::vector<model::DetachedCosignature>& cosignatures) {
				ASSERT_EQ(m_cosignatures.size(), cosignatures.size());

				for (auto i = 0u; i < m_cosignatures.size(); ++i) {
					auto message = "cosignature at " + std::to_string(i);
					EXPECT_EQ(m_cosignatures[i].Signer, cosignatures[i].Signer) << message;
					EXPECT_EQ(m_cosignatures[i].Signature, cosignatures[i].Signature) << message;
					EXPECT_EQ(m_cosignatures[i].Scheme, cosignatures[i].Scheme) << message;
					EXPECT_EQ(m_cosignatures[i].ParentHash, cosignatures[i].ParentHash) << message;
				}
			}

		private:
			std::vector<model::DetachedCosignature> m_cosignatures;
		};
	}

	TEST(TEST_CLASS, SplitCosignedTransactionInfosDoesNotForwardAnythingForEmptyInfos) {
		// Act:
		auto capture = SplitAndCapture({});

		// Assert: nothing should have been forwarded
		EXPECT_TRUE(capture.TransactionRanges.empty());
		EXPECT_TRUE(capture.Cosignatures.empty());
	}

	TEST(TEST_CLASS, SplitCosignedTransactionInfosForwardsTransactionsSameVersion) {
		// Arrange:
		auto transactionInfos = CosignedTransactionInfos();
		TransactionGenerator generator;
		generator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 1);
		generator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 0);
		generator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 3);

		// Act:
		auto capture = SplitAndCapture(transactionInfos);

		// Assert: a single transaction range should have been forwarded
		ASSERT_EQ(1u, capture.TransactionRanges.size());
		generator.assertData(capture.TransactionRanges[0]);

		// - no cosignatures should have been forwarded
		EXPECT_TRUE(capture.Cosignatures.empty());
	}

	TEST(TEST_CLASS, SplitCosignedTransactionInfosForwardsTransactionsMultiVersion) {
		// Arrange:
		auto transactionInfos = CosignedTransactionInfos();
		TransactionGenerator generator;
		generator.addData<model::AggregateTransactionExtendedDescriptor>(transactionInfos, 1);
		generator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 0);
		generator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 3);

		// Act:
		auto capture = SplitAndCapture(transactionInfos);

		// Assert: a single transaction range should have been forwarded
		ASSERT_EQ(1u, capture.TransactionRanges.size());
		generator.assertData(capture.TransactionRanges[0]);

		// - no cosignatures should have been forwarded
		EXPECT_TRUE(capture.Cosignatures.empty());
	}

	TEST(TEST_CLASS, SplitCosignedTransactionInfosForwardsCosignaturesSameVersion) {
		// Arrange:
		auto transactionInfos = CosignedTransactionInfos();
		CosignatureGenerator generator;
		generator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 1);
		generator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 3);
		generator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 2);

		// Act:
		auto capture = SplitAndCapture(transactionInfos);

		// Assert: no transactions should have been forwarded
		EXPECT_TRUE(capture.TransactionRanges.empty());

		// - all cosignatures should have been forwarded
		EXPECT_EQ(6u, capture.Cosignatures.size());
		generator.assertData(capture.Cosignatures);
	}

	TEST(TEST_CLASS, SplitCosignedTransactionInfosForwardsCosignaturesMultiVersion) {
		// Arrange:
		auto transactionInfos = CosignedTransactionInfos();
		CosignatureGenerator generator;
		generator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 1);
		generator.addData<model::AggregateTransactionExtendedDescriptor>(transactionInfos, 3);
		generator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 2);

		// Act:
		auto capture = SplitAndCapture(transactionInfos);

		// Assert: no transactions should have been forwarded
		EXPECT_TRUE(capture.TransactionRanges.empty());

		// - all cosignatures should have been forwarded
		EXPECT_EQ(6u, capture.Cosignatures.size());
		generator.assertData(capture.Cosignatures);
	}

	TEST(TEST_CLASS, SplitCosignedTransactionInfosForwardsTransactionsAndCosignaturesSameVersion) {
		// Arrange: interleave cosignature and transaction infos
		auto transactionInfos = CosignedTransactionInfos();
		CosignatureGenerator cosignatureGenerator;
		TransactionGenerator transactionGenerator;
		transactionGenerator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 1);
		cosignatureGenerator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 3);
		transactionGenerator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 2);
		cosignatureGenerator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 1);
		transactionGenerator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 0);
		cosignatureGenerator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 2);

		// Act:
		auto capture = SplitAndCapture(transactionInfos);

		// Assert: a single transaction range should have been forwarded
		ASSERT_EQ(1u, capture.TransactionRanges.size());
		transactionGenerator.assertData(capture.TransactionRanges[0]);

		// - all cosignatures should have been forwarded
		EXPECT_EQ(6u, capture.Cosignatures.size());
		cosignatureGenerator.assertData(capture.Cosignatures);
	}

	// Notice: Aggregate V1 transactions do not support V2 cosignatures
	TEST(TEST_CLASS, SplitCosignedTransactionInfosForwardsTransactionsAndCosignaturesMultiVersion) {
		// Arrange: interleave cosignature and transaction infos
		auto transactionInfos = CosignedTransactionInfos();
		CosignatureGenerator cosignatureGenerator;
		TransactionGenerator transactionGenerator;
		transactionGenerator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 1);
		cosignatureGenerator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 3);
		transactionGenerator.addData<model::AggregateTransactionExtendedDescriptor>(transactionInfos, 2);
		cosignatureGenerator.addData<model::AggregateTransactionExtendedDescriptor>(transactionInfos, 1);
		cosignatureGenerator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 1);
		transactionGenerator.addData<model::AggregateTransactionExtendedDescriptor>(transactionInfos, 0);
		cosignatureGenerator.addData<model::AggregateTransactionRawDescriptor>(transactionInfos, 2);

		// Act:
		auto capture = SplitAndCapture(transactionInfos);

		// Assert: a single transaction range should have been forwarded
		ASSERT_EQ(1u, capture.TransactionRanges.size());
		transactionGenerator.assertData(capture.TransactionRanges[0]);

		// - all cosignatures should have been forwarded
		EXPECT_EQ(7u, capture.Cosignatures.size());
		cosignatureGenerator.assertData(capture.Cosignatures);
	}

	// endregion
}}
