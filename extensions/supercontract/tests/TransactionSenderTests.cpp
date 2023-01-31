/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/TransactionSender.h"
#include "tests/TestHarness.h"
#include "plugins/txes/supercontract_v2/src/model/SuccessfulEndBatchExecutionTransaction.h"

namespace catapult { namespace contract {

#define TEST_CLASS SupercontractTransactionSender

	namespace{
		auto CreateTransactionSender(handlers::TransactionRangeHandler transactionRangeHandler) {
			return TransactionSender(
				crypto::KeyPair::FromString("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"),
				config::ImmutableConfiguration::Uninitialized(),
				ExecutorConfiguration::Uninitialized(),
				transactionRangeHandler);
		}

		auto CreateSuccessfulEndBatchExecutionTransaction(){
			sirius::contract::SuccessfulEndBatchExecutionTransactionInfo transactionInfo;

			const Key contractKey({11});
			const auto batchIndex = 1;
			const auto automaticExecutionsCheckedUpTo = 1;
			sirius::contract::SuccessfulBatchInfo successfulBatchInfo;
			std::vector<sirius::contract::SuccessfulCallExecutionInfo> callsExecutionInfos;
			std::vector<sirius::contract::Proofs> proofs;
			std::vector<sirius::contract::ExecutorKey> executorKeys;
			std::vector<sirius::Signature> signatures;

			//successfulBatchInfo
			successfulBatchInfo.m_storageHash = sirius::contract::StorageHash("a");
			successfulBatchInfo.m_metaFilesSize = 50;
			successfulBatchInfo.m_usedStorageSize = 50;
			successfulBatchInfo.m_PoExVerificationInfo = sirius::crypto::CurvePoint();

			//callsExecutionInfo
			std::vector<sirius::contract::CallExecutorParticipation> callExecutorParticipants;
			callExecutorParticipants.emplace_back(sirius::contract::CallExecutorParticipation{10, 10});
			callExecutorParticipants.emplace_back(sirius::contract::CallExecutorParticipation{20, 20});

			callsExecutionInfos.emplace_back(
				sirius::contract::SuccessfulCallExecutionInfo{
					sirius::contract::CallId("b"),
					true,
					1,
					1,
					sirius::contract::TransactionHash("c"),
					callExecutorParticipants
				}
 			);
			callsExecutionInfos.emplace_back(
				sirius::contract::SuccessfulCallExecutionInfo{
					sirius::contract::CallId("d"),
					true,
					1,
					1,
					sirius::contract::TransactionHash("e"),
					callExecutorParticipants
				}
			);

			//proofs
			sirius::contract::BatchProof batchProof;
			sirius::contract::TProof tProof;
			proofs.emplace_back(
				sirius::contract::Proofs{
					0,
					tProof,
					batchProof
				}
			);
			proofs.emplace_back(
				sirius::contract::Proofs{
					0,
					tProof,
					batchProof
				}
			);

			//executorKeys
			auto executorKey1 = sirius::contract::ExecutorKey({ 1 });
			auto executorKey2 = sirius::contract::ExecutorKey({ 2 });
			executorKeys.emplace_back(executorKey1);
			executorKeys.emplace_back(executorKey2);

			//signatures
			auto signature1 = sirius::Signature({ 3 });
			auto signature2 = sirius::Signature({ 4 });
			signatures.emplace_back(signature1);
			signatures.emplace_back(signature2);

			transactionInfo.m_contractKey = contractKey.array();
			transactionInfo.m_batchIndex = batchIndex;
			transactionInfo.m_automaticExecutionsCheckedUpTo = automaticExecutionsCheckedUpTo;
			transactionInfo.m_successfulBatchInfo = successfulBatchInfo;
			transactionInfo.m_callsExecutionInfo = callsExecutionInfos;
			transactionInfo.m_proofs = proofs;
			transactionInfo.m_executorKeys = executorKeys;
			transactionInfo.m_signatures = signatures;

			return transactionInfo;
		}
	}

	TEST(TEST_CLASS, SendSuccessfulEndBatchExecutionTransaction) {
		std::shared_ptr<model::Transaction> pTransaction;
		auto transactionRangeHandler = [&pTransaction](model::AnnotatedEntityRange<catapult::model::Transaction>&& range) {
			pTransaction = model::EntityRange<model::Transaction>::ExtractEntitiesFromRange(std::move(range.Range))[0];
		};
		auto testee = CreateTransactionSender(transactionRangeHandler);
		std::vector<Key> expectedPublicKeys{ Key({ 1 }), Key({ 2 }) };
		std::vector<Signature> expectedSignatures{ Signature({ 3 }), Signature({ 4 }) };
		std::vector<model::RawProofOfExecution> expectedProofs;
		std::vector<model::ExtendedCallDigest> expectedCallDigests;
		std::vector<model::CallPayment> expectedCallPayments;

		//expectedProofs
		sirius::contract::Proofs proof;
		expectedProofs.emplace_back(
			model::RawProofOfExecution{
				0,
				proof.m_batchProof.m_T.toBytes(),
				proof.m_batchProof.m_r.array(),
				proof.m_tProof.m_F.toBytes(),
				proof.m_tProof.m_k.array()
			}
		);
		expectedProofs.emplace_back(
			model::RawProofOfExecution{
				0,
				proof.m_batchProof.m_T.toBytes(),
				proof.m_batchProof.m_r.array(),
				proof.m_tProof.m_F.toBytes(),
				proof.m_tProof.m_k.array()
			}
		);

		//expectedCallDigest
		expectedCallDigests.emplace_back(
			model::ExtendedCallDigest{
				sirius::contract::CallId("b").array(),
				true,
				Height(1),
				1,
				sirius::contract::TransactionHash("c").array()
			}
 		);
		expectedCallDigests.emplace_back(
			model::ExtendedCallDigest{
				sirius::contract::CallId("d").array(),
				true,
				Height(1),
				1,
				sirius::contract::TransactionHash("e").array()
			}
		);

		//expectedPayment
		expectedCallPayments.emplace_back(model::CallPayment{Amount(10), Amount(10)});
		expectedCallPayments.emplace_back(model::CallPayment{Amount(20), Amount(20)});

		testee.sendSuccessfulEndBatchExecutionTransaction(CreateSuccessfulEndBatchExecutionTransaction());

		//Assert
		auto& transaction = static_cast<const model::SuccessfulEndBatchExecutionTransaction&>(*pTransaction);
		EXPECT_EQ_MEMORY(expectedPublicKeys.data(), transaction.PublicKeysPtr(), expectedPublicKeys.size() * Key_Size);
		EXPECT_EQ_MEMORY(expectedSignatures.data(), transaction.SignaturesPtr(), expectedSignatures.size() * Key_Size);
		EXPECT_EQ_MEMORY(expectedProofs.data(), transaction.ProofsOfExecutionPtr(), expectedProofs.size()  * sizeof(model::RawProofOfExecution));
		EXPECT_EQ_MEMORY(expectedCallDigests.data(), transaction.CallDigestsPtr(), expectedCallDigests.size() * sizeof(model::ExtendedCallDigest));
		EXPECT_EQ_MEMORY(expectedCallPayments.data(), transaction.CallPaymentsPtr(), expectedCallPayments.size() * sizeof(model::CallPayment));
	}


}}