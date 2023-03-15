/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"

namespace catapult::observers {

	using Notification = model::ManualCallNotification<1>;

	DEFINE_OBSERVER(ManualCall, model::ManualCallNotification<1>, [](const model::ManualCallNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ManualCall)");

		auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
		auto contractIt = contractCache.find(notification.ContractKey);
		auto& contractEntry = contractIt.get();

		state::ContractCall contractCall;
		contractCall.CallId = notification.CallId;
		contractCall.Caller = notification.Caller;
		contractCall.FileName = notification.FileName;
		contractCall.FunctionName = notification.FunctionName;
		contractCall.ActualArguments = notification.ActualArguments;
		contractCall.ExecutionCallPayment = notification.ExecutionCallPayment;
		contractCall.DownloadCallPayment = notification.DownloadCallPayment;

		contractCall.ServicePayments.reserve(notification.ServicePayments.size());
		for (const auto& [mosaicId, amount]: notification.ServicePayments) {
			contractCall.ServicePayments.push_back(state::ServicePayment{mosaicId, amount});
		}

		contractCall.BlockHeight = context.Height;

		contractEntry.requestedCalls().push_back(std::move(contractCall));
	})

}
