/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/NetworkConfigCache.h"

namespace catapult { namespace observers {

	using Notification = model::NetworkConfigNotification<1>;

	namespace {
		template<typename TNotification>
		void ObserveNetworkConfigNotification(const TNotification& notification, const ObserverContext& context) {
			auto& cache = context.Cache.sub<cache::NetworkConfigCache>();
			Height height;
			if constexpr(std::is_same_v<TNotification, model::NetworkConfigNotification<1>>)
				height = Height{context.Height.unwrap() + notification.ApplyHeightDelta.unwrap()};
			else {
				if (notification.UpdateType == model::NetworkUpdateType::Delta)
					height = Height { context.Height.unwrap() + notification.ApplyHeight };
				else {
					height = Height(notification.ApplyHeight);
				}
			}
			if (NotifyMode::Commit == context.Mode) {
				cache.insert(state::NetworkConfigEntry(height,
													   std::string((const char*)notification.BlockChainConfigPtr, notification.BlockChainConfigSize),
													   std::string((const char*)notification.SupportedEntityVersionsPtr, notification.SupportedEntityVersionsSize)));
			} else {
				if (cache.contains(height))
					cache.remove(height);
			}
		}
	}
	DECLARE_OBSERVER(NetworkConfig, model::NetworkConfigNotification<1>)() {
		return MAKE_OBSERVER(NetworkConfig, model::NetworkConfigNotification<1>, ObserveNetworkConfigNotification<model::NetworkConfigNotification<1>>);
	}
	DECLARE_OBSERVER(NetworkConfigV2, model::NetworkConfigNotification<2>)() {
		return MAKE_OBSERVER(NetworkConfigV2, model::NetworkConfigNotification<2>, ObserveNetworkConfigNotification<model::NetworkConfigNotification<2>>);
	}
}}
