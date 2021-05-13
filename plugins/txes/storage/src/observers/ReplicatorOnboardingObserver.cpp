/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(ReplicatorOnboarding, model::ReplicatorOnboardingNotification<1>, [](const model::ReplicatorOnboardingNotification<1>& notification, ObserverContext& context) {
		auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		if (NotifyMode::Commit == context.Mode) {
			state::ReplicatorEntry replicatorEntry(notification.PublicKey);
			replicatorEntry.setCapacity(notification.Capacity);
			replicatorCache.insert(replicatorEntry);
		} else {
			if (replicatorCache.contains(notification.PublicKey))
				replicatorCache.remove(notification.PublicKey);
		}
	});
}}
