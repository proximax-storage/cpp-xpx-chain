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
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (BatchCalls)");

			auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
			auto contractIt = contractCache.find(notification.ContractKey);
			auto& contractEntry = contractIt.get();

			auto& accountCache = context.Cache.sub<cache::AccountStateCache>();
			auto contractAccountIt = accountCache.find(contractEntry.key());
			auto& contractAccountEntry = contractAccountIt.get();

			auto& batch = (--contractEntry.batches().end())->second;

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

					auto servicePaymentsReceiverKey = digest.Status == 0
															  ? notification.ContractKey : requestedCallIt->Caller;
					auto servicePaymentsReceiverAccountIt = accountCache.find(servicePaymentsReceiverKey);
					auto& servicePaymentsReceiverAccountEntry = servicePaymentsReceiverAccountIt.get();

					for (const auto& [mosaicId, amount] : requestedCallIt->ServicePayments) {
						auto resolvedMosaicId = context.Resolvers.resolve(mosaicId);
						servicePaymentsReceiverAccountEntry.Balances.credit(resolvedMosaicId, amount);
					}

					requestedCallIt++;
				}
				else {
					call.Caller = Key();
				}
				if (digest.Status == 0 && digest.ReleasedTransactionHash != Hash256()) {
					contractEntry.releasedTransactions().insert(digest.ReleasedTransactionHash);
				}
				batch.CompletedCalls.push_back(call);
			}

			auto readOnlyCache = context.Cache.toReadOnly();
			auto executorsNumber = driveBrowser->getOrderedReplicatorsCount(readOnlyCache, contractEntry.driveKey());
			auto contractCreator = contractEntry.creator();

			auto scMosaicId = config::GetUnresolvedSuperContractMosaicId(context.Config.Immutable);
			auto streamingMosaicId = config::GetUnresolvedStreamingMosaicId(context.Config.Immutable);

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
					scRefund = Amount((automaticExecutionsInfo.AutomaticExecutionCallPayment - call.ExecutionWork).unwrap() * executorsNumber);
					scRefund = Amount((automaticExecutionsInfo.AutomaticDownloadCallPayment - call.DownloadWork).unwrap() * executorsNumber);
					refundReceiver = contractCreator;
					automaticExecutionsInfo.AutomatedExecutionsNumber--;
				}
				liquidityProvider->debitMosaics(
						context,
						contractEntry.executionPaymentKey(),
						refundReceiver,
						scMosaicId,
						scRefund);
				liquidityProvider->debitMosaics(
						context,
						contractEntry.executionPaymentKey(),
						refundReceiver,
						streamingMosaicId,
						streamingRefund);
			}

			if (automaticExecutionsInfo.AutomatedExecutionsNumber == 0) {
				automaticExecutionsInfo.AutomaticExecutionsPrepaidSince.reset();
			}

			if (contractEntry.batches().size() == 1) {
				if (automaticExecutionsInfo.AutomatedExecutionsNumber > 0) {
					automaticExecutionsInfo.AutomaticExecutionsPrepaidSince = context.Height;
				}
			}
		}))
	}
}
