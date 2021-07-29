/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(ReplicatorOnboarding, model::ReplicatorOnboardingNotification<1>, [](const model::ReplicatorOnboardingNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ReplicatorOnboarding)");

	  	auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		state::ReplicatorEntry replicatorEntry(notification.PublicKey);
		replicatorEntry.setCapacity(notification.Capacity);
	  	replicatorEntry.setBlsKey(notification.BlsKey);
		replicatorCache.insert(replicatorEntry);

	  	auto& blsKeysCache = context.Cache.sub<cache::BlsKeysCache>();
	  	state::BlsKeysEntry blsKeysEntry(notification.BlsKey);
	  	blsKeysEntry.setKey(notification.PublicKey);
	  	blsKeysCache.insert(blsKeysEntry);
	});
}}
