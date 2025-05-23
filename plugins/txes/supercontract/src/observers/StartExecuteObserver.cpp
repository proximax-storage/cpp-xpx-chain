/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/SuperContractCache.h"

namespace catapult { namespace observers {

	using Notification = model::StartExecuteNotification<1>;

	DEFINE_OBSERVER(StartExecute, Notification, ([](const auto& notification, const ObserverContext& context) {
		auto& superContractCache = context.Cache.sub<cache::SuperContractCache>();
		auto superContractCacheIter = superContractCache.find(notification.SuperContract);
		auto& superContractEntry = superContractCacheIter.get();
		if (NotifyMode::Commit == context.Mode) {
			superContractEntry.incrementExecutionCount();
		} else {
			superContractEntry.decrementExecutionCount();
		}
	}));
}}
