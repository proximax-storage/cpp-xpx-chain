/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"
#include "src/utils/MathUtils.h"

namespace catapult::observers {

	using Notification = model::BatchCallsNotification<1>;

	DEFINE_OBSERVER(BatchCalls, Notification , [](const Notification & notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (FinishDownload)");

		auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
		auto contractIt = contractCache.find(notification.ContractKey);
		auto& contractEntry = contractIt.get();

		state::Batch batch;
		batch.PoExVerificationInformation = notification.ProofOfExecutionVerificationInfo;
		for (uint i = 0; i < notification.Digests.size(); i++) {
			const auto& digest = notification.Digests[i];
			const auto& payments = notification.PaymentOpinions[i];
			auto executionWork = utils::median(payments.ExecutionWork);
			auto downloadWork = utils::median(payments.DownloadWork);
			state::CompletedCall call;
			call.CallId = digest.CallId;
			call.Manual = digest.Manual;
			call.Success = digest.Success;
			call.ExecutionWork = executionWork;
			call.DownloadWork = downloadWork;
			batch.CompletedCalls.push_back(call);
		}

		contractEntry.batches().push_back(batch);
	})

}
