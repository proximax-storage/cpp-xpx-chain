/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/TransactionSender.h"
#include "tests/TestHarness.h"
#include "plugins/txes/supercontract_v2/src/model/SuccessfulEndBatchExecutionTransaction.h"
#include "plugins/txes/supercontract_v2/src/model/UnsuccessfulEndBatchExecutionTransaction.h"
#include "plugins/txes/supercontract_v2/src/model/EndBatchExecutionSingleTransaction.h"
#include "plugins/txes/supercontract_v2/src/model/SynchronizationSingleTransaction.h"

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

			Key contractKey({ 11 });
			uint64_t batchIndex = 1;
			uint64_t automaticExecutionsCheckedUpTo = 1;
			sirius::contract::SuccessfulBatchInfo successfulBatchInfo;
			std::vector<sirius::contract::SuccessfulCallExecutionInfo> callsExecutionInfos;
			std::vector<sirius::contract::Proofs> proofs;
			std::vector<sirius::contract::ExecutorKey> executorKeys;
			std::vector<sirius::Signature> signatures;

			//successfulBatchInfo
			successfulBatchInfo.m_storageHash = sirius::contract::StorageHash("0123456789ABCDEF0123456789ABCDEF");
			successfulBatchInfo.m_metaFilesSize = 50;
			successfulBatchInfo.m_usedStorageSize = 50;
			successfulBatchInfo.m_PoExVerificationInfo = sirius::crypto::CurvePoint();

			//callsExecutionInfo
			std::vector<sirius::contract::CallExecutorParticipation> callExecutorParticipants;
			callExecutorParticipants.emplace_back(sirius::contract::CallExecutorParticipation{10, 20});
			callExecutorParticipants.emplace_back(sirius::contract::CallExecutorParticipation{10, 20});

			callsExecutionInfos.emplace_back(
				sirius::contract::SuccessfulCallExecutionInfo{
					sirius::contract::CallId("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"),
					true,
					1,
					1,
					sirius::contract::TransactionHash("1123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"),
					callExecutorParticipants
				}
 			);
			callsExecutionInfos.emplace_back(
				sirius::contract::SuccessfulCallExecutionInfo{
					sirius::contract::CallId("2123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"),
					true,
					1,
					1,
					sirius::contract::TransactionHash("3123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"),
					callExecutorParticipants
				}
			);

			//proofs
			sirius::crypto::Scalar scalar(std::array<uint8_t ,32>{5});
			sirius::crypto::CurvePoint curvePoint = sirius::crypto::CurvePoint::BasePoint() * scalar;
			sirius::contract::TProof tProof = {curvePoint, scalar};
			sirius::contract::BatchProof batchProof = {curvePoint, scalar};
			sirius::contract::Proofs proof = {0, tProof, batchProof};
			proofs.emplace_back(proof);
			proofs.emplace_back(proof);

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
		// Arrange:
		std::shared_ptr<model::Transaction> pTransaction;
		auto transactionRangeHandler = [&pTransaction](model::AnnotatedEntityRange<catapult::model::Transaction>&& range) {
			pTransaction = model::EntityRange<model::Transaction>::ExtractEntitiesFromRange(std::move(range.Range))[0];
		};
		auto testee = CreateTransactionSender(transactionRangeHandler);
		Key expectedContractKey({ 11 });
		uint64_t expectedBatchIndex = 1;
		Hash256 expectedStorageHash = sirius::contract::StorageHash("0123456789ABCDEF0123456789ABCDEF").array();
		uint64_t expectedUsedSizeBytes = 50;
		uint64_t expectedMetaDilesSizeBytes = 50;
		std::array<uint8_t, 32>&& expectedProofOfExecutionVerificationInformation = sirius::crypto::CurvePoint().toBytes();
		auto expectedAutomaticExecutionCheckedUpTo = Height(1);
		auto expectedCosignersNumber = 2;
		auto expectedCallsNumber = 2;
		std::vector<Key> expectedPublicKeys{ Key({ 1 }), Key({ 2 }) };
		std::vector<Signature> expectedSignatures{ Signature({ 3 }), Signature({ 4 }) };
		std::vector<model::RawProofOfExecution> expectedProofs;
		std::vector<model::ExtendedCallDigest> expectedCallDigests;
		std::vector<model::CallPayment> expectedCallPayments;

		//expectedProofs
		sirius::crypto::Scalar scalar(std::array<uint8_t ,32>{5});
		sirius::crypto::CurvePoint curvePoint = sirius::crypto::CurvePoint::BasePoint() * scalar;
		sirius::contract::TProof tProof = {curvePoint, scalar};
		sirius::contract::BatchProof batchProof = {curvePoint, scalar};
		sirius::contract::Proofs proof = {0, tProof, batchProof};
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
				sirius::contract::CallId("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ").array(),
				true,
				Height(1),
				1,
				sirius::contract::TransactionHash("1123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ").array()
			}
 		);
		expectedCallDigests.emplace_back(
			model::ExtendedCallDigest{
				sirius::contract::CallId("2123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ").array(),
				true,
				Height(1),
				1,
				sirius::contract::TransactionHash("3123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ").array()
			}
		);

		//expectedPayment
		expectedCallPayments.emplace_back(model::CallPayment{Amount(10), Amount(20)});
		expectedCallPayments.emplace_back(model::CallPayment{Amount(10), Amount(20)});

		testee.sendSuccessfulEndBatchExecutionTransaction(CreateSuccessfulEndBatchExecutionTransaction());

		// Assert
		auto& transaction = static_cast<const model::SuccessfulEndBatchExecutionTransaction&>(*pTransaction);
		EXPECT_EQ(expectedContractKey, transaction.ContractKey);
		EXPECT_EQ(expectedBatchIndex, transaction.BatchId);
		EXPECT_EQ(expectedStorageHash, transaction.StorageHash);
		EXPECT_EQ(expectedUsedSizeBytes, transaction.UsedSizeBytes);
		EXPECT_EQ(expectedMetaDilesSizeBytes, transaction.MetaFilesSizeBytes);
		EXPECT_EQ(expectedProofOfExecutionVerificationInformation, transaction.ProofOfExecutionVerificationInformation);
		EXPECT_EQ(expectedAutomaticExecutionCheckedUpTo, transaction.AutomaticExecutionsNextBlockToCheck);
		EXPECT_EQ(expectedCosignersNumber, transaction.CosignersNumber);
		EXPECT_EQ(expectedCallsNumber, transaction.CallsNumber);
		EXPECT_EQ_MEMORY(expectedPublicKeys.data(), transaction.PublicKeysPtr(), expectedPublicKeys.size() * Key_Size);
		EXPECT_EQ_MEMORY(expectedSignatures.data(), transaction.SignaturesPtr(), expectedSignatures.size() * Key_Size);
		EXPECT_EQ_MEMORY(expectedProofs.data(), transaction.ProofsOfExecutionPtr(), expectedProofs.size()  * sizeof(model::RawProofOfExecution));
		EXPECT_EQ_MEMORY(expectedCallDigests.data(), transaction.CallDigestsPtr(), expectedCallDigests.size() * sizeof(model::ExtendedCallDigest));
		EXPECT_EQ_MEMORY(expectedCallPayments.data(), transaction.CallPaymentsPtr(), expectedCallPayments.size() * sizeof(model::CallPayment));
	}

	namespace {
		auto CreateUnsuccessfulEndBatchExecutionTransaction(){
			sirius::contract::UnsuccessfulEndBatchExecutionTransactionInfo transactionInfo;

			Key contractKey({ 11 });
			auto batchIndex = 1;
			auto automaticExecutionsCheckedUpTo = 1;
			std::vector<sirius::contract::UnsuccessfulCallExecutionInfo> callsExecutionInfos;
			std::vector<sirius::contract::Proofs> proofs;
			std::vector<sirius::contract::ExecutorKey> executorKeys;
			std::vector<sirius::Signature> signatures;

			//callsExecutionInfos
			std::vector<sirius::contract::CallExecutorParticipation> callExecutorParticipants;
			callExecutorParticipants.emplace_back(sirius::contract::CallExecutorParticipation{10, 20});
			callExecutorParticipants.emplace_back(sirius::contract::CallExecutorParticipation{10, 20});

			callsExecutionInfos.emplace_back(
				sirius::contract::UnsuccessfulCallExecutionInfo{
					sirius::contract::CallId("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"),
					true,
					1,
					callExecutorParticipants
				}
			);
			callsExecutionInfos.emplace_back(
				sirius::contract::UnsuccessfulCallExecutionInfo{
					sirius::contract::CallId("1123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"),
					true,
					1,
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
			transactionInfo.m_callsExecutionInfo = callsExecutionInfos;
			transactionInfo.m_proofs = proofs;
			transactionInfo.m_executorKeys = executorKeys;
			transactionInfo.m_signatures = signatures;

			return transactionInfo;
		}

		TEST(TEST_CLASS, SendUnsuccessfulEndBatchExecutionTransaction) {
			// Arrange:
			std::shared_ptr<model::Transaction> pTransaction;
			auto transactionRangeHandler = [&pTransaction](model::AnnotatedEntityRange<catapult::model::Transaction>&& range) {
				pTransaction = model::EntityRange<model::Transaction>::ExtractEntitiesFromRange(std::move(range.Range))[0];
			};
			auto testee = CreateTransactionSender(transactionRangeHandler);
			Key expectedContractKey({ 11 });
			uint64_t expectedBatchIndex = 1;
			auto expectedAutomaticExecutionCheckedUpTo = Height(1);
			auto expectedCosignersNumber = 2;
			auto expectedCallsNumber = 2;
			std::vector<Key> expectedPublicKeys{ Key({ 1 }), Key({ 2 }) };
			std::vector<Signature> expectedSignatures{ Signature({ 3 }), Signature({ 4 }) };
			std::vector<model::RawProofOfExecution> expectedProofs;
			std::vector<model::ShortCallDigest> expectedCallDigests;
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
				model::ShortCallDigest{
					sirius::contract::CallId("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ").array(),
					true,
					Height(1),
				}
			);
			expectedCallDigests.emplace_back(
				model::ShortCallDigest{
					sirius::contract::CallId("1123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ").array(),
					true,
					Height(1),
				}
			);

			//expectedPayment
			expectedCallPayments.emplace_back(model::CallPayment{Amount(10), Amount(20)});
			expectedCallPayments.emplace_back(model::CallPayment{Amount(10), Amount(20)});

			testee.sendUnsuccessfulEndBatchExecutionTransaction(CreateUnsuccessfulEndBatchExecutionTransaction());

			// Assert
			auto& transaction = static_cast<const model::UnsuccessfulEndBatchExecutionTransaction&>(*pTransaction);
			EXPECT_EQ(expectedContractKey, transaction.ContractKey);
			EXPECT_EQ(expectedBatchIndex, transaction.BatchId);
			EXPECT_EQ(expectedAutomaticExecutionCheckedUpTo, transaction.AutomaticExecutionsNextBlockToCheck);
			EXPECT_EQ(expectedCosignersNumber, transaction.CosignersNumber);
			EXPECT_EQ(expectedCallsNumber, transaction.CallsNumber);
			EXPECT_EQ_MEMORY(expectedPublicKeys.data(), transaction.PublicKeysPtr(), expectedPublicKeys.size() * Key_Size);
			EXPECT_EQ_MEMORY(expectedSignatures.data(), transaction.SignaturesPtr(), expectedSignatures.size() * Key_Size);
			EXPECT_EQ_MEMORY(expectedProofs.data(), transaction.ProofsOfExecutionPtr(), expectedProofs.size()  * sizeof(model::RawProofOfExecution));
			EXPECT_EQ_MEMORY(expectedCallDigests.data(), transaction.CallDigestsPtr(), expectedCallDigests.size() * sizeof(model::ShortCallDigest));
			EXPECT_EQ_MEMORY(expectedCallPayments.data(), transaction.CallPaymentsPtr(), expectedCallPayments.size() * sizeof(model::CallPayment));
		}
	}

	namespace{
		auto CreateEndBatchExecutionSingleTransaction(){
			sirius::contract::EndBatchExecutionSingleTransactionInfo transactionInfo;

			Key contractKey({ 11 });
			auto batchIndex = 1;
			sirius::contract::Proofs proof;

			transactionInfo.m_contractKey = contractKey.array();
			transactionInfo.m_batchIndex = batchIndex;
			transactionInfo.m_proofOfExecution = proof;

			return transactionInfo;
		}
	}

	TEST(TEST_CLASS, SendEndBatchExecutionSingleTransaction) {
		// Arrange:
		std::shared_ptr<model::Transaction> pTransaction;
		auto transactionRangeHandler = [&pTransaction](model::AnnotatedEntityRange<catapult::model::Transaction>&& range) {
			pTransaction = model::EntityRange<model::Transaction>::ExtractEntitiesFromRange(std::move(range.Range))[0];
		};
		auto testee = CreateTransactionSender(transactionRangeHandler);
		Key expectedContractKey({ 11 });
		uint64_t expectedBatchIndex = 1;
		model::RawProofOfExecution expectedProof;

		testee.sendEndBatchExecutionSingleTransaction(CreateEndBatchExecutionSingleTransaction());

		// Assert
		auto& transaction = static_cast<const model::EndBatchExecutionSingleTransaction&>(*pTransaction);
		EXPECT_EQ(expectedContractKey, transaction.ContractKey);
		EXPECT_EQ(expectedBatchIndex, transaction.BatchId);
//		EXPECT_EQ(expectedProof, transaction.ProofOfExecution);
	}

	namespace{
		auto CreateSynchronizationSingleTransaction(){
			sirius::contract::SynchronizationSingleTransactionInfo transactionInfo;

			Key contractKey({ 11 });
			auto batchIndex = 1;

			transactionInfo.m_contractKey = contractKey.array();
			transactionInfo.m_batchIndex = batchIndex;

			return transactionInfo;
		}
	}

	TEST(TEST_CLASS, SendSynchronizationSingleTransaction) {
		// Arrange:
		std::shared_ptr<model::Transaction> pTransaction;
		auto transactionRangeHandler = [&pTransaction](model::AnnotatedEntityRange<catapult::model::Transaction>&& range) {
			pTransaction = model::EntityRange<model::Transaction>::ExtractEntitiesFromRange(std::move(range.Range))[0];
		};
		auto testee = CreateTransactionSender(transactionRangeHandler);
		Key expectedContractKey({ 11 });
		uint64_t expectedBatchIndex = 1;

		testee.sendEndBatchExecutionSingleTransaction(CreateEndBatchExecutionSingleTransaction());

		// Assert
		auto& transaction = static_cast<const model::SynchronizationSingleTransaction&>(*pTransaction);
		EXPECT_EQ(expectedContractKey, transaction.ContractKey);
		EXPECT_EQ(expectedBatchIndex, transaction.BatchId);
	}
}}