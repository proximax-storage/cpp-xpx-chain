/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/utils/ContractUtils.h"

namespace catapult { namespace validators {

	using Notification = model::BatchCallsNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(BatchCalls, [](const Notification& notification, const ValidatorContext& context) {

		if (notification.Digests.empty()) {
			return Failure_SuperContract_Empty_Batch;
		}

		const auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
		auto contractIt = contractCache.find(notification.ContractKey);

		// For the moment we should have already been checked that the contract exists
		const auto& contractEntry = contractIt.get();

		auto requestedCallIt = contractEntry.requestedCalls().begin();

		const auto& automaticExecutionsInfo = contractEntry.automaticExecutionsInfo();
		auto automaticExecutionsLeft = automaticExecutionsInfo.AutomaticExecutionsPrepaidSince
											   ? 1
											   : 0;
		for (uint i = 0; i < notification.Digests.size(); i++) {
			const auto& digest = notification.Digests[i];
			const auto& payments = notification.PaymentOpinions[i];
			if (digest.Manual) {
				if (requestedCallIt == contractEntry.requestedCalls().end()) {
					return Failure_SuperContract_Manual_Calls_Are_Not_Requested;
				}
				if (digest.CallId != requestedCallIt->CallId) {
					return Failure_SuperContract_Invalid_Call_Id;
				}
				for (const auto& payment: payments.ExecutionWork) {
					if (payment > requestedCallIt->ExecutionCallPayment) {
						return Failure_SuperContract_Execution_Work_Is_Too_Large;
					}
				}
				for (const auto& payment: payments.DownloadWork) {
					if (payment > requestedCallIt->DownloadCallPayment) {
						return Failure_SuperContract_Download_Work_Is_Too_Large;
					}
				}
				requestedCallIt++;
			}
			else {
				if (automaticExecutionsLeft == 0) {
					return Failure_SuperContract_Automatic_Calls_Are_Not_Requested;
				}
				auto enabledSince =
						utils::automaticExecutionsEnabledSince(contractEntry, context.Height, context.Config);

				if (digest.Block < enabledSince) {
					return Failure_SuperContract_Outdated_Automatic_Execution;
				}
				
				for (const auto& payment: payments.ExecutionWork) {
					if (payment > automaticExecutionsInfo.AutomaticExecutionCallPayment) {
						return Failure_SuperContract_Execution_Work_Is_Too_Large;
					}
				}
				for (const auto& payment: payments.DownloadWork) {
					if (payment > automaticExecutionsInfo.AutomaticDownloadCallPayment) {
						return Failure_SuperContract_Download_Work_Is_Too_Large;
					}
				}
				automaticExecutionsLeft--;
			}
		}

		return ValidationResult::Success;
	})

}}
