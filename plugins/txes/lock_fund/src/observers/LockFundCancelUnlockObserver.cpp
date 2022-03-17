/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/LockFundCache.h"
#include "Observers.h"
#include "src/config/LockFundConfiguration.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(LockFundCancelUnlock, model::LockFundCancelUnlockNotification<1>, ([](const auto& notification, const ObserverContext& context) {
		auto& lockFundCache = context.Cache.sub<cache::LockFundCache>();
		if(context.Mode == NotifyMode::Commit)
			lockFundCache.disable(notification.Sender, notification.TargetHeight);
		else
			lockFundCache.recover(notification.Sender, notification.TargetHeight);

	}));
}}
