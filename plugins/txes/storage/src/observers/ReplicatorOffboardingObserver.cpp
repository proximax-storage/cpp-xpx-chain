/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(ReplicatorOffboarding, model::ReplicatorOffboardingNotification<1>, [](const model::ReplicatorOffboardingNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ReplicatorOffboarding)");

	  	auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		replicatorCache.remove(notification.PublicKey);
	});
}}
