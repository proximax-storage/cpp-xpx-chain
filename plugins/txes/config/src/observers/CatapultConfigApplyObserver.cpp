/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/CatapultConfigCache.h"

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<1>;

	DECLARE_OBSERVER(CatapultConfigApply, Notification)(plugins::PluginManager& manager) {
		return MAKE_OBSERVER(CatapultConfigApply, Notification, [&manager](const auto&, const ObserverContext& context) {
			auto& cache = context.Cache.sub<cache::CatapultConfigCache>();
			if (cache.contains(context.Height)) {
				if (NotifyMode::Commit == context.Mode) {
					const auto& entry = cache.find(context.Height).get();
					manager.configHolder()->SetBlockChainConfig(context.Height, entry.blockChainConfig());
				} else {
					manager.configHolder()->RemoveBlockChainConfig(context.Height);
				}
			}
		});
	}
}}
