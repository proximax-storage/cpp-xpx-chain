/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/LockFundCache.h"
#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"


namespace catapult { namespace observers {

	/// Note: Consider removing records in batches instead
	DEFINE_OBSERVER(LockFundBlock, model::BlockNotification<1>, ([](const auto& notification, const ObserverContext& context) {
		if (context.Mode == NotifyMode::Rollback)
			return;
		const model::NetworkConfiguration& config = context.Config.Network;
	  	if (context.Height.unwrap() - config.MaxRollbackBlocks <= 0)
			return;

		auto targetHeight = context.Height-Height(config.MaxRollbackBlocks);
		auto& lockFundCache = context.Cache.sub<cache::LockFundCache>();
		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto heightRecord = lockFundCache.find(targetHeight);
		auto record = heightRecord.tryGet();
		if(record)
		{
			// Unlock amounts
			for(auto& record : record->LockFundRecords)
			{
				auto account = accountStateCache.find(record.first).get();
				for(auto& mosaic : record.second.Get())
				{
					account.Balances.unlock(mosaic.first, mosaic.second);
				}
			}

			// Clear records
			lockFundCache.remove(targetHeight);
		}
	}));
}}
