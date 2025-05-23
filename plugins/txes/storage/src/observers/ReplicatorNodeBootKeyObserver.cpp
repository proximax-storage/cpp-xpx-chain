/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/utils/StorageUtils.h"
#include "src/cache/BootKeyReplicatorCache.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(ReplicatorNodeBootKey, model::ReplicatorNodeBootKeyNotification<1>, [](const auto& notification, auto& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ReplicatorNodeBootKey)");

		auto& replicatorCache = context.Cache.template sub<cache::ReplicatorCache>();
		auto replicatorIter = replicatorCache.find(notification.ReplicatorKey);
		auto& replicatorEntry = replicatorIter.get();
		replicatorEntry.setNodeBootKey(notification.NodeBootKey);

		auto& bootKeyReplicatorCache = context.Cache.template sub<cache::BootKeyReplicatorCache>();
		bootKeyReplicatorCache.insert(state::BootKeyReplicatorEntry(notification.NodeBootKey, notification.ReplicatorKey));
	});
}}
