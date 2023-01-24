/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "TransactionSender.h"
#include "sdk/src/builders/SuccessfulEndBatchExecutionBuilder.h"
#include "sdk/src/builders/UnsuccessfulEndBatchExecutionBuilder.h"
#include "sdk/src/builders/EndBatchExecutionSingleBuilder.h"
#include "sdk/src/extensions/TransactionExtensions.h"
#include "catapult/model/EntityRange.h"
#include "catapult/model/RangeTypes.h"
#include "catapult/model/EntityHasher.h"

namespace catapult { namespace contract {

	Hash256 TransactionSender::sendSuccessfulEndBatchExecutionTransaction(const sirius::contract::SuccessfulEndBatchExecutionTransactionInfo& transactionInfo) {
		CATAPULT_LOG(debug) << "sending successful end batch execution transaction initiated by " << Hash256(transactionInfo.m_contractKey.array());
		std::vector<Key> publicKeys;
		std::vector<Signature> signatures;
		std::vector<model::RawProofOfExecution> proofs;
		std::vector<model::ExtendedCallDigest> callDigests;
		std::vector<model::CallPayment> callPayments;

		for(const auto& key:transactionInfo.m_executorKeys){
			publicKeys.emplace_back(key.array());
		}

		for(const auto& signature:transactionInfo.m_signatures){
			signatures.emplace_back(signature.array());
		}

		for(const auto& proof:transactionInfo.m_proofs){
			proofs.push_back(model::RawProofOfExecution{
					proof.m_initialBatch,
					proof.m_batchProof.m_T.toBytes(),
					proof.m_batchProof.m_r.array(),
					proof.m_tProof.m_F.toBytes(),
					proof.m_tProof.m_k.array()
			});
		}

		for(const auto& call:transactionInfo.m_callsExecutionInfo){
			callDigests.push_back(model::ExtendedCallDigest{
				call.m_callId.array(),
				call.m_manual,
				Height(call.m_block),
				static_cast<int16_t>(call.m_callExecutionStatus),
				call.m_releasedTransaction.array()
			});

			for(const auto& participation:call.m_executorsParticipation){
				callPayments.push_back(model::CallPayment{
						Amount(participation.m_scConsumed),
						Amount(participation.m_smConsumed)
				});
			}
		}

		// build and send transaction
		builders::SuccessfulEndBatchExecutionBuilder builder(m_networkIdentifier, m_keyPair.publicKey());
		builder.setContractKey(transactionInfo.m_contractKey.array());
		builder.setBatchId(transactionInfo.m_batchIndex);
		builder.setStorageHash(transactionInfo.m_successfulBatchInfo.m_storageHash.array());
		builder.setUsedSizeBytes(transactionInfo.m_successfulBatchInfo.m_usedStorageSize);
		builder.setMetaFilesSizeBytes(transactionInfo.m_successfulBatchInfo.m_metaFilesSize);
		builder.setProofOfExecutionVerificationInformation(transactionInfo.m_successfulBatchInfo.m_PoExVerificationInfo.toBytes());
		builder.setAutomaticExecutionsNextBlockToCheck(Height(transactionInfo.m_automaticExecutionsCheckedUpTo));
		builder.setCosignersNumber(transactionInfo.m_executorKeys.size());
		builder.setCallsNumber(transactionInfo.m_callsExecutionInfo.size());
		builder.setPublicKeys(std::move(publicKeys));
		builder.setSignatures(std::move(signatures));
		builder.setProofsOfExecution(std::move(proofs));
		builder.setCallDigests(std::move(callDigests));
		builder.setCallPayments(std::move(callPayments));
		auto pTransaction = utils::UniqueToShared(builder.build());
		pTransaction->Deadline = utils::NetworkTime() + Timestamp(m_executorConfig.TransactionTimeout.millis());
		send(pTransaction);

		return model::CalculateHash(*pTransaction, m_generationHash);
	}

	Hash256 TransactionSender::sendUnsuccessfulEndBatchExecutionTransaction(const sirius::contract::UnsuccessfulEndBatchExecutionTransactionInfo& transactionInfo) {
		CATAPULT_LOG(debug) << "sending unsuccessful end batch execution transaction initiated by " << Hash256(transactionInfo.m_contractKey.array());
		std::vector<Key> publicKeys;
		std::vector<Signature> signatures;
		std::vector<model::RawProofOfExecution> proofs;
		std::vector<model::ShortCallDigest> callDigests;
		std::vector<model::CallPayment> callPayments;

		for(const auto& key:transactionInfo.m_executorKeys){
			publicKeys.emplace_back(key.array());
		}

		for(const auto& signature:transactionInfo.m_signatures){
			signatures.emplace_back(signature.array());
		}

		for(const auto& proof:transactionInfo.m_proofs){
			proofs.push_back(model::RawProofOfExecution{
					proof.m_initialBatch,
					proof.m_batchProof.m_T.toBytes(),
					proof.m_batchProof.m_r.array(),
					proof.m_tProof.m_F.toBytes(),
					proof.m_tProof.m_k.array()
			});
		}

		for(const auto& call:transactionInfo.m_callsExecutionInfo){
			callDigests.push_back(model::ShortCallDigest{
				call.m_callId.array(),
				call.m_manual,
				Height(call.m_block)
			});

			for(const auto& participation:call.m_executorsParticipation){
				callPayments.push_back(model::CallPayment{
						Amount(participation.m_scConsumed),
						Amount(participation.m_smConsumed)
				});
			}
		}

		builders::UnsuccessfulEndBatchExecutionBuilder builder(m_networkIdentifier, m_keyPair.publicKey());
		builder.setContractKey(transactionInfo.m_contractKey.array());
		builder.setBatchId(transactionInfo.m_batchIndex);
		builder.setAutomaticExecutionsNextBlockToCheck(Height(transactionInfo.m_automaticExecutionsCheckedUpTo));
		builder.setCosignersNumber(transactionInfo.m_executorKeys.size());
		builder.setCallsNumber(transactionInfo.m_callsExecutionInfo.size());
		builder.setPublicKeys(std::move(publicKeys));
		builder.setSignatures(std::move(signatures));
		builder.setProofsOfExecution(std::move(proofs));
		builder.setCallDigests(std::move(callDigests));
		builder.setCallPayments(std::move(callPayments));
		auto pTransaction = utils::UniqueToShared(builder.build());
		pTransaction->Deadline = utils::NetworkTime() + Timestamp(m_executorConfig.TransactionTimeout.millis());
		send(pTransaction);

		return model::CalculateHash(*pTransaction, m_generationHash);
	}
	Hash256 TransactionSender::sendEndBatchExecutionSingleTransaction(const sirius::contract::EndBatchExecutionSingleTransactionInfo& transactionInfo) {
		CATAPULT_LOG(debug) << "sending end batch execution single transaction initiated by " << Hash256(transactionInfo.m_contractKey.array());

		model::RawProofOfExecution proof{
			transactionInfo.m_proofOfExecution.m_initialBatch,
			transactionInfo.m_proofOfExecution.m_batchProof.m_T.toBytes(),
			transactionInfo.m_proofOfExecution.m_batchProof.m_r.array(),
			transactionInfo.m_proofOfExecution.m_tProof.m_F.toBytes(),
			transactionInfo.m_proofOfExecution.m_tProof.m_k.array()
		};
		builders::EndBatchExecutionSingleBuilder builder(m_networkIdentifier, m_keyPair.publicKey());
		builder.setContractKey(transactionInfo.m_contractKey.array());
		builder.setBatchId(transactionInfo.m_batchIndex);
		builder.setProofOfExecution(proof);
		auto pTransaction = utils::UniqueToShared(builder.build());
		pTransaction->Deadline = utils::NetworkTime() + Timestamp(m_executorConfig.TransactionTimeout.millis());
		send(pTransaction);

		return model::CalculateHash(*pTransaction, m_generationHash);
	}

	void TransactionSender::send(std::shared_ptr<model::Transaction> pTransaction) {
		extensions::TransactionExtensions(m_generationHash).sign(m_keyPair, *pTransaction);
		auto range = model::TransactionRange::FromEntity(pTransaction);
		m_transactionRangeHandler({std::move(range), m_keyPair.publicKey()});
	}
}}