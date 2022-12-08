/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <catapult/crypto/Hashes.h>
#include "SuccessfulEndBatchExecutionTransactionPlugin.h"
#include "catapult/model/SupercontractNotifications.h"
#include "src/model/SuccessfulEndBatchExecutionTransaction.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "catapult/model/NotificationSubscriber.h"
#include "src/model/InternalSuperContractNotifications.h"
#include "src/utils/SwapUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {

		template<class T>
		void pushBytes(std::vector<uint8_t>& buffer, const T& data) {
			const auto* pBegin = reinterpret_cast<const uint8_t*>(&data);
			buffer.insert(buffer.end(), pBegin, pBegin + sizeof(data));
		}

		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					std::vector<uint8_t> commonData;
					pushBytes(commonData, transaction.ContractKey);
					pushBytes(commonData, transaction.BatchId);
					pushBytes(commonData, transaction.StorageHash);
					pushBytes(commonData, transaction.ProofOfExecutionVerificationInformation);
					for (uint j = 0; j < transaction.CallsNumber; j++) {
						pushBytes(commonData, transaction.CallDigestsPtr()[j]);
					}

					std::vector<model::Opinion> opinions;
					opinions.reserve(transaction.CosignersNumber);
					for (uint i = 0; i < transaction.CosignersNumber; i++) {
						std::vector<uint8_t> data;
						const model::RawProofOfExecution& proofOfExecution = transaction.ProofsOfExecutionPtr()[i];
						pushBytes(data, proofOfExecution);
						for (uint j = 0; j < transaction.CallsNumber; j++) {
							pushBytes(data, transaction.CallPaymentsPtr()[i * transaction.CallsNumber + j]);
						}
						opinions.emplace_back(transaction.PublicKeysPtr()[i], transaction.SignaturesPtr()[i], std::move(data));
					}

					sub.notify(model::OpinionSignatureNotification<1>(commonData, opinions));

					sub.notify(model::EndBatchExecutionNotification<1>(transaction.ContractKey, transaction.BatchId));

					std::set<Key> cosigners;
					for (uint i = 0; i < transaction.CosignersNumber; i++) {
						cosigners.insert(transaction.PublicKeysPtr()[i]);
					}

					crypto::CurvePoint poExVerificationInformation;
					poExVerificationInformation.fromBytes(transaction.ProofOfExecutionVerificationInformation);
					sub.notify(model::SuccessfulBatchExecutionNotification<1>(
							transaction.ContractKey,
							transaction.BatchId,
							transaction.StorageHash,
							poExVerificationInformation,
							cosigners));

					std::map<Key, ProofOfExecution> proofs;
					for (uint i = 0; i < transaction.CosignersNumber; i++) {
						const auto& rawProof = transaction.ProofsOfExecutionPtr()[i];
						model::ProofOfExecution proof;
						proof.StartBatchId = rawProof.StartBatchId;

						proof.T.fromBytes(rawProof.T);
						proof.R = crypto::Scalar(rawProof.R);
						proof.F.fromBytes(rawProof.F);
						proof.K = crypto::Scalar(rawProof.K);
						proofs[transaction.PublicKeysPtr()[i]] = proof;
					}
					sub.notify(model::ProofOfExecutionNotification<1>(transaction.ContractKey, proofs));

					std::vector<model::CallPaymentOpinion> callPaymentOpinions;
					callPaymentOpinions.reserve(transaction.CosignersNumber);

					for (uint j = 0; j < transaction.CallsNumber; j++) {
						std::vector<Amount> executionWork;
						std::vector<Amount> downloadWork;
						for (uint i = 0; i < transaction.CosignersNumber; i++) {
							for (uint j = 0; j < transaction.CallsNumber; j++) {
								executionWork.push_back(transaction.CallPaymentsPtr()[i * transaction.CallsNumber + j].ExecutionPayment);
								downloadWork.push_back(transaction.CallPaymentsPtr()[i * transaction.CallsNumber + j].DownloadPayment);
							}
							callPaymentOpinions.push_back(CallPaymentOpinion{executionWork, downloadWork});
						}
					}

					std::vector<CallDigest> callDigests;
					callDigests.reserve(transaction.CallsNumber);
					for (uint j = 0; j < transaction.CallsNumber; j++) {
						callDigests.push_back(transaction.CallDigestsPtr()[j]);
					}


					sub.notify(model::BatchCallsNotification<1>(transaction.ContractKey, callDigests, callPaymentOpinions));
					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of SuccessfulEndBatchExecutionTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(SuccessfulEndBatchExecution, Default, CreatePublisher, config::ImmutableConfiguration)
}}
