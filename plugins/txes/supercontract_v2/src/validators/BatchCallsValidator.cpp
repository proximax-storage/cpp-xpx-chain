/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::BatchCallsNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(BatchCalls, [](const Notification& notification, const ValidatorContext& context) {

		const auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
		auto contractIt = contractCache.find(notification.ContractKey);

		// For the moment we should have already been checked that the contract exists
		const auto& contractEntry = contractIt.get();

		auto requestedCallIt = contractEntry.requestedCalls().begin();

		const auto& automaticExecutionsInfo = contractEntry.automaticExecutionsInfo();
		auto automaticExecutionsLeft = automaticExecutionsInfo.m_automaticExecutionsEnabledSince
											   ? automaticExecutionsInfo.m_automatedExecutionsNumber
											   : 0;
		for (uint i = 0; i < notification.Digests.size(); i++) {
			const auto& digest = notification.Digests[i];
			const auto& payments = notification.PaymentOpinions[i];
			if (digest.Manual) {
				if (requestedCallIt == contractEntry.requestedCalls().end()) {
					return Failure_SuperContract_Manual_Calls_Are_Not_Requested;
				}
				else if (digest.CallId != requestedCallIt->CallId) {
					return Failure_SuperContract_Invalid_Call_Id;
				}
				for (const auto& payment: payments.ExecutionWork) {
					if (payment > requestedCallIt->ExecutionCallPayment) {
						return Failure_SuperContract_ExecutionWorkIsTooLarge;
					}
				}
				for (const auto& payment: payments.DownloadWork) {
					if (payment > requestedCallIt->DownloadCallPayment) {
						return Failure_SuperContract_DownloadWorkIsTooLarge;
					}
				}
				requestedCallIt++;
			}
			else {
				if (automaticExecutionsLeft == 0) {
					return Failure_SuperContract_Automatic_Calls_Are_Not_Requested;
				}
				for (const auto& payment: payments.ExecutionWork) {
					if (payment > automaticExecutionsInfo.m_automatedExecutionCallPayment) {
						return Failure_SuperContract_ExecutionWorkIsTooLarge;
					}
				}
				for (const auto& payment: payments.DownloadWork) {
					if (payment > automaticExecutionsInfo.m_automatedDownloadCallPayment) {
						return Failure_SuperContract_DownloadWorkIsTooLarge;
					}
				}
				automaticExecutionsLeft--;
			}
		}

		return ValidationResult::Success;
	})

}}
