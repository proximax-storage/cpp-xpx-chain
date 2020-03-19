/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/SuperContractCache.h"
#include "plugins/txes/operation/src/cache/OperationCache.h"
#include "catapult/observers/ObserverUtils.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(ExpiredExecution, model::BlockNotification<1>, [](const auto&, auto& context) {
		auto& operationCache = context.Cache.template sub<cache::OperationCache>();
		auto& superContractCache = context.Cache.template sub<cache::SuperContractCache>();

		operationCache.processUnusedExpiredLocks(context.Height, [&context, &superContractCache](
				const auto& operationEntry) {
			if (operationEntry.Executors.size() != 1)
				return;

			auto superContractKey = *operationEntry.Executors.begin();
			if (!superContractCache.contains(superContractKey))
				return;

			auto superContractCacheIter = superContractCache.find(superContractKey);
			auto& superContractEntry = superContractCacheIter.get();
			if (NotifyMode::Commit == context.Mode) {
				superContractEntry.decrementExecutionCount();
			} else {
				superContractEntry.incrementExecutionCount();
			}
		});
	});
}}
