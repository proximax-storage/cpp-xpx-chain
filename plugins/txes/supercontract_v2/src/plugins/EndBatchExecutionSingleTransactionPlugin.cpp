/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <catapult/crypto/Hashes.h>
#include "EndBatchExecutionSingleTransactionPlugin.h"
#include "catapult/model/SupercontractNotifications.h"
#include "src/model/EndBatchExecutionSingleTransaction.h"
#include "src/model/InternalSuperContractNotifications.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "catapult/model/NotificationSubscriber.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					sub.notify(model::ContractStateUpdateNotification<1>(transaction.ContractKey));

					sub.notify(model::BatchExecutionSingleNotification<1>(
							transaction.ContractKey, transaction.BatchId, transaction.Signer));

					std::map<Key, ProofOfExecution> proofs;
					const auto& rawProof = transaction.ProofOfExecution;
					model::ProofOfExecution proof;
					proof.StartBatchId = rawProof.StartBatchId;

					proof.T.fromBytes(rawProof.T);
					proof.R = crypto::Scalar(rawProof.R);
					proof.F.fromBytes(rawProof.F);
					proof.K = crypto::Scalar(rawProof.K);
					proofs[transaction.Signer] = proof;
					sub.notify(model::ProofOfExecutionNotification<1>(transaction.ContractKey, proofs));

					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of EndBatchExecutionSingleTransaction: "
										<< transaction.EntityVersion();
				}
			};
		}
	} // namespace

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(
			EndBatchExecutionSingle,
			Default,
			CreatePublisher,
			config::ImmutableConfiguration)
}} // namespace catapult::plugins
