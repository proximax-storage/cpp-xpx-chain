/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#include "plugins/services/globalstore/src/state/BaseConverters.h"
#include "plugins/services/globalstore/src/cache/GlobalStoreCache.h"
#include "src/cache/LockFundCache.h"
#include "Observers.h"

#include "catapult/model/Address.h"



namespace catapult { namespace observers {

		using Notification = model::BlockNotification<1>;
		DEFINE_OBSERVER(LockFundInstall, Notification, [](const Notification& notification, const ObserverContext& context) {
			if(context.Height != context.Config.ActivationHeight || context.Config.PreviousConfiguration == nullptr)
				return;

			// Check if plugin is enabled in new configuration and disabled in old configuration
			auto newConfig = context.Config.Network.GetPluginConfiguration<config::LockFundConfiguration>();
			auto oldConfig = context.Config.PreviousConfiguration->Network.GetPluginConfiguration<config::LockFundConfiguration>();
			if(newConfig.Enabled && !oldConfig.Enabled) {
				auto& globalStore = context.Cache.sub<cache::GlobalStoreCache>();
				auto recordIter = globalStore.find(config::LockFundPluginInstalled_GlobalKey);
				if (NotifyMode::Commit == context.Mode) {
					if(!recordIter.tryGet())
					{
						state::GlobalEntry installedRecord(config::LockFundPluginInstalled_GlobalKey, context.Height.unwrap(), state::PluginInstallConverter());
						state::GlobalEntry totalStaked(config::TotalStaked_GlobalKey, 0, state::Uint64Converter());
						globalStore.insert(installedRecord);
						globalStore.insert(totalStaked);

					}
				} else {
					if(recordIter.tryGet() && recordIter.get().Get<state::PluginInstallConverter>() == context.Height.unwrap())
					{
						globalStore.remove(config::LockFundPluginInstalled_GlobalKey);
						globalStore.remove(config::TotalStaked_GlobalKey);
					}

				}
			}
		});
}}
