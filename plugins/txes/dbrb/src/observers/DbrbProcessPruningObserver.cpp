/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/DbrbViewCache.h"
#include "src/cache/DbrbViewFetcherImpl.h"

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<1>;

	DECLARE_OBSERVER(DbrbProcessPruning, Notification)(const std::shared_ptr<cache::DbrbViewFetcherImpl>& pDbrbViewFetcher) {
		return MAKE_OBSERVER(DbrbProcessPruning, Notification, [pDbrbViewFetcher](const Notification&, ObserverContext& context) {
			auto& cache = context.Cache.sub<cache::DbrbViewCache>();
			auto expiredProcesses = pDbrbViewFetcher->getExpiredDbrbProcesses(context.Timestamp);
			if (NotifyMode::Rollback == context.Mode && !expiredProcesses.empty())
			  	CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DbrbProcessPruning)");

			for (const auto& processId : expiredProcesses)
				cache.remove(processId);
        })
	};
}}
