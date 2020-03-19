/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/SuperContractCache.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(Deactivate, model::DeactivateNotification<1>, [](const auto& notification, const ObserverContext& context) {
		auto& superContractCache = context.Cache.sub<cache::SuperContractCache>();
		auto superContractCacheIter = superContractCache.find(notification.SuperContract);
		auto& superContractEntry = superContractCacheIter.get();
		if (NotifyMode::Commit == context.Mode) {
			superContractEntry.setState(state::SuperContractState::DeactivatedByParticipant);
		} else {
			superContractEntry.setState(state::SuperContractState::Active);
		}
	});
}}
