/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"

namespace catapult::observers {

	using Notification = model::ProofOfExecutionNotification<1>;

	DECLARE_OBSERVER(ProofOfExecution, Notification)(
			const std::unique_ptr<LiquidityProviderExchangeObserver>& liquidityProvider,
			const std::unique_ptr<StorageExternalManagementObserver>& storageExternalManager) {
		return MAKE_OBSERVER(ProofOfExecution, Notification, ([&liquidityProvider, &storageExternalManager](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ProofOfExecution)");

			auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
			auto contractIt = contractCache.find(notification.ContractKey);
			auto& contractEntry = contractIt.get();

			for (const auto& [key, proof] : notification.Proofs) {
				auto& executor = contractEntry.executorsInfo()[key];
				executor.PoEx.StartBatchId = proof.StartBatchId;
				executor.PoEx.T = proof.T;
				executor.PoEx.R = proof.R;
			}

			const auto& scMosaicId = config::GetUnresolvedSuperContractMosaicId(context.Config.Immutable);
			const auto& streamingMosaicId = config::GetUnresolvedStreamingMosaicId(context.Config.Immutable);

			Amount scPayment(0);
			Amount streamingPayment(0);
			for (const auto& [key, _] : notification.Proofs) {
				auto& executor = contractEntry.executorsInfo()[key];
				auto startBatch = std::max(executor.PoEx.StartBatchId, executor.NextBatchToApprove);
				for (auto batchId = startBatch; batchId < contractEntry.nextBatchId(); batchId++) {
					const auto& batch = contractEntry.batches()[batchId];
					for (const auto& call : batch.CompletedCalls) {
						scPayment = scPayment + call.ExecutionWork;
						streamingPayment = streamingPayment + call.DownloadWork;
					}
				}
				liquidityProvider->debitMosaics(
						context, contractEntry.executionPaymentKey(), key, scMosaicId, scPayment);
				liquidityProvider->debitMosaics(
						context, contractEntry.executionPaymentKey(), key, streamingMosaicId, streamingPayment);
				executor.NextBatchToApprove = contractEntry.nextBatchId();
			}

			std::set<Key> executors;
			for (const auto& [key, _]: notification.Proofs) {
				executors.insert(key);
			}
			storageExternalManager->addToConfirmedStorage(context, contractEntry.driveKey(), executors);
		}))
	}
}
