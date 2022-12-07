/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"
#include "src/utils/MathUtils.h"
#include "src/catapult/observers/LiquidityProviderExchangeObserver.h"
#include "src/catapult/state/DriveStateBrowser.h"
#include "src/catapult/cache/ReadOnlyCatapultCache.h"

namespace catapult::observers {

	using Notification = model::BatchCallsNotification<1>;

	DECLARE_OBSERVER(BatchCalls, Notification)(const std::unique_ptr<LiquidityProviderExchangeObserver>& liquidityProvider,
	 											const std::unique_ptr<state::DriveStateBrowser>& driveBrowser) {
		return MAKE_OBSERVER(BatchCalls, Notification, ([&liquidityProvider, &driveBrowser](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (FinishDownload)");

			auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
			auto contractIt = contractCache.find(notification.ContractKey);
			auto& contractEntry = contractIt.get();

			state::Batch batch;
			batch.PoExVerificationInformation = notification.ProofOfExecutionVerificationInfo;

			auto requestedCallIt = contractEntry.requestedCalls().begin();

			for (uint i = 0; i < notification.Digests.size(); i++) {
				const auto& digest = notification.Digests[i];
				const auto& payments = notification.PaymentOpinions[i];
				auto executionWork = utils::median(payments.ExecutionWork);
				auto downloadWork = utils::median(payments.DownloadWork);
				state::CompletedCall call;
				call.CallId = digest.CallId;
				call.Status = digest.Status;
				call.ExecutionWork = executionWork;
				call.DownloadWork = downloadWork;
				if (digest.Manual) {
					call.Caller = requestedCallIt->Caller;
					requestedCallIt++;
				}
				else {
					call.Caller = Key();
				}
				batch.CompletedCalls.push_back(call);
			}

			const auto& readOnlyCache = context.Cache.toReadOnly();
			auto executorsNumber = driveBrowser->getOrderedReplicatorsCount(readOnlyCache, contractEntry.driveKey());
			auto driveOwner = driveBrowser->getDriveOwner(readOnlyCache, contractEntry.driveKey());

			const auto& scMosaicId = config::GetUnresolvedSuperContractMosaicId(context.Config.Immutable);
			const auto& streamingMosaicId = config::GetUnresolvedStreamingMosaicId(context.Config.Immutable);

			auto& automaticExecutionsInfo = contractEntry.automaticExecutionsInfo();

			for (const auto& call: batch.CompletedCalls) {

				Amount scRefund;
				Amount streamingRefund;
				Key refundReceiver;

				if (call.Caller != Key()) {
					// Manual Call
					auto requestedCall = std::move(contractEntry.requestedCalls().front());
					contractEntry.requestedCalls().pop_front();
					scRefund = Amount((requestedCall.ExecutionCallPayment - call.ExecutionWork).unwrap() * executorsNumber);
					streamingRefund = Amount((requestedCall.DownloadCallPayment - call.DownloadWork).unwrap() * executorsNumber);
					refundReceiver = call.Caller;
				}
				else {
					scRefund = Amount((automaticExecutionsInfo.m_automatedExecutionCallPayment - call.ExecutionWork).unwrap() * executorsNumber);
					scRefund = Amount((automaticExecutionsInfo.m_automatedDownloadCallPayment - call.DownloadWork).unwrap() * executorsNumber);
					refundReceiver = call.Caller;
					automaticExecutionsInfo.m_automatedExecutionsNumber--;
				}
				liquidityProvider->debitMosaics(
						context,
						contractEntry.executionPaymentKey(),
						call.Caller,
						scMosaicId,
						scRefund);
				liquidityProvider->debitMosaics(
						context,
						contractEntry.executionPaymentKey(),
						call.Caller,
						streamingMosaicId,
						streamingRefund);
			}

			// TODO Consider this
			if (automaticExecutionsInfo.m_automatedExecutionsNumber == 0) {
				automaticExecutionsInfo.m_automaticExecutionsEnabledSince.reset();
			}

			contractEntry.batches().push_back(batch);
		}))
	}
}
