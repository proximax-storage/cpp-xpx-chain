/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/CommitteeCache.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"
#include "catapult/crypto/KeyPair.h"

namespace catapult { namespace observers {

	DECLARE_OBSERVER(RemoveDbrbProcessByNetwork, model::RemoveDbrbProcessByNetworkNotification<1>)(const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector) {
		return MAKE_OBSERVER(RemoveDbrbProcessByNetwork, model::RemoveDbrbProcessByNetworkNotification<1>, ([pAccountCollector](const auto& notification, auto& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (RemoveDbrbProcessByNetwork)");

			auto& cache = context.Cache.template sub<cache::CommitteeCache>();
			for (const auto& [key, accountData] : pAccountCollector->accounts()) {
				if (accountData.BootKey == notification.ProcessId) {
					auto iter = cache.find(key);
					auto pEntry = iter.tryGet();
					if (pEntry)
						pEntry->setExpirationTime(Timestamp(0));
				}
			}
		}));
	}
}}
