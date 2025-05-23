/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/DriveCache.h"

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<1>;

	DECLARE_OBSERVER(DriveCacheBlockPruning, Notification)() {
		return MAKE_OBSERVER(DriveCacheBlockPruning, Notification, [](const Notification&, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				return;

            const model::NetworkConfiguration &config = context.Config.Network;

            auto gracePeriod = BlockDuration(config.MaxRollbackBlocks);
            if (context.Height.unwrap() <= gracePeriod.unwrap())
                return;

            auto pruneHeight = Height(context.Height.unwrap() - gracePeriod.unwrap());
            auto &cache = context.Cache.sub<cache::DriveCache>();
            cache.prune(pruneHeight, context);
        })
	};
}}
