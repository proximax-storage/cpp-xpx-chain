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

					crypto::CurvePoint poExVerificationInformation;
					poExVerificationInformation.fromBytes(transaction.ProofOfExecutionVerificationInformation);

					std::vector<model::CallOpinion> callOpinions;
					callOpinions.reserve(transaction.CosignersNumber);
					for (uint i = 0; i < transaction.CosignersNumber; i++) {

						const model::RawProofOfExecution& proofOfExecution = transaction.ProofsOfExecutionPtr()[i];

						crypto::CurvePoint T;
						T.fromBytes(proofOfExecution.T);
						crypto::Scalar r(proofOfExecution.R);

						crypto::CurvePoint F;
						F.fromBytes(proofOfExecution.F);
						crypto::Scalar k(proofOfExecution.K);

						model::ProofOfExecution poEx{proofOfExecution.FirstBatchId, T, r, F, k};

						std::vector<uint64_t> executionWork;
						for (uint j = 0; j < transaction.CallsNumber; j++) {
							executionWork.push_back(transaction.CallPaymentsPtr()[i * transaction.CallsNumber + j].ExecutionPayment);
						}
						std::vector<uint64_t> downloadWork;
						for (uint j = 0; j < transaction.CallsNumber; j++) {
							downloadWork.push_back(transaction.CallPaymentsPtr()[i * transaction.CallsNumber + j].DownloadPayment);
						}

						callOpinions.push_back(model::CallOpinion{transaction.PublicKeysPtr()[i], poEx, executionWork, downloadWork});
					}

					std::vector<CallDigest> callDigests;
					callDigests.reserve(transaction.CallsNumber);
					for (uint j = 0; j < transaction.CallsNumber; j++) {
						callDigests.push_back(transaction.CallDigestsPtr()[j]);
					}

					sub.notify(SuccessfulBatchExecutionNotification<1>(transaction.ContractKey, transaction.BatchId, transaction.StorageHash, poExVerificationInformation, callOpinions, callDigests));

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
