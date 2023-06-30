/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/AccountStateCacheUtils.h"
#include "plugins/txes/upgrade/src/model/BlockchainUpgradeNotifications.h"
namespace catapult { namespace observers {


	using Notification = model::AccountV2UpgradeNotification<1>;


namespace {

	void ExecuteObservation(const Notification& notification, const ObserverContext& context) {
		/// We will move the entire state of account V1 to the new target account.

		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto& cache = context.Cache.sub<cache::ExchangeCache>();

		if(NotifyMode::Commit == context.Mode)
		{
			auto accountEntry = cache.find(notification.Signer).tryGet();
			if(accountEntry) {
				auto newEntry = accountEntry->upgrade(notification.NewAccountPublicKey);
				cache.insert(newEntry);
				cache.remove(accountEntry->owner());
			}
		}
		else
		{
			auto accountEntry = cache.find(notification.NewAccountPublicKey).tryGet();
			if(accountEntry) {
				auto newEntry = accountEntry->upgrade(notification.Signer);
				cache.insert(newEntry);
				cache.remove(accountEntry->owner());
			}

		}

	}
}


	DECLARE_OBSERVER(AccountV2OfferUpgrade, Notification)() {
		return MAKE_OBSERVER(AccountV2OfferUpgrade, Notification, ExecuteObservation);
	}
}}
