/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include <map>
#include "Observers.h"
#include "src/utils/MathUtils.h"
#include "src/catapult/observers/LiquidityProviderExchangeObserver.h"
#include "src/catapult/state/DriveStateBrowser.h"
#include "src/catapult/cache/ReadOnlyCatapultCache.h"

namespace catapult::observers {

	using Notification = model::EndBatchExecutionNotification<1>;

	DECLARE_OBSERVER(EndBatchExecution, Notification)(const std::unique_ptr<state::DriveStateBrowser>& driveBrowser) {
		return MAKE_OBSERVER(EndBatchExecution, Notification, ([&driveBrowser](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (EndBatchExecution)");

			auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
			auto contractIt = contractCache.find(notification.ContractKey);
			auto& contractEntry = contractIt.get();

//			auto& executorsInfo = contractEntry.executorsInfo();
//			auto minIt = std::min_element(
//					executorsInfo.cbegin(),
//					executorsInfo.cend(),
//					[](const auto& left, const auto& right) {
//						return left.second.NextBatchToApprove < right.second.NextBatchToApprove;
//					});
//
//			auto minNextBatchToApprove =
//					minIt != executorsInfo.end() ? minIt->second.NextBatchToApprove : 0;
//
//			while (!executorsInfo.empty() && exe

			contractEntry.batches().emplace(contractEntry.nextBatchId(), state::Batch{});
			contractEntry.automaticExecutionsInfo().AutomaticExecutionsNextBlockToCheck = notification.AutomaticExecutionsNextBlockToCheck;
		 }))
	}
}
