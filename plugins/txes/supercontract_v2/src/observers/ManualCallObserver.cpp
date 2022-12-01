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
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (FinishDownload)");

		auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
		auto contractIt = contractCache.find(notification.ContractKey);
		auto& contractEntry = contractIt.get();

		state::ContractCall contractCall;
		contractCall.CallId = notification.CallId;
		contractCall.Caller = notification.Caller;
		contractCall.FileName = notification.FileName;
		contractCall.FunctionName = contractCall.FunctionName;
		contractCall.ActualArguments = contractCall.ActualArguments;
		contractCall.ExecutionCallPayment = contractCall.ExecutionCallPayment;
		contractCall.DownloadCallPayment = contractCall.DownloadCallPayment;
		contractCall.ServicePayments = contractCall.ServicePayments;

		contractEntry.requestedCalls().push(std::move(contractCall));
	})

}
