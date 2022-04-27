/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/LockFundCache.h"
#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"


namespace catapult { namespace observers {

	namespace {
		void ProcessLockfundUpdate(const model::BlockNotification<1>& notification, const ObserverContext& context)
		{
			const model::NetworkConfiguration& config = context.Config.Network;
			auto& lockFundCache = context.Cache.sub<cache::LockFundCache>();
			auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
			if (context.Mode == NotifyMode::Commit)
			{
				auto activeHeightRecord = lockFundCache.find(context.Height);
				auto record = activeHeightRecord.tryGet();
				if(record)
				{
					// Unlock amounts
					for(auto& irecord : record->LockFundRecords)
					{
						if(irecord.second.Active())
						{
							auto &account = accountStateCache.find(irecord.first).get();
							for(auto& mosaic : irecord.second.Get())
							{
								account.Balances.unlock(mosaic.first, mosaic.second, context.Height);
							}
						}
					}
					// Disable records
					lockFundCache.disable(context.Height);
				}
				//Clear aged records
				if (context.Height.unwrap() - config.MaxRollbackBlocks <= 0)
					return;
				auto clearHeight = context.Height-Height(config.MaxRollbackBlocks);
				auto heightRecord = lockFundCache.find(clearHeight);
				auto agedRecord = heightRecord.tryGet();
				if(agedRecord)
				{
					// Clear records
					lockFundCache.remove(clearHeight);
				}
			}
			else
			{
				auto activeHeightRecord = lockFundCache.find(context.Height);
				auto record = activeHeightRecord.tryGet();
				if(record)
				{
					// Disable records
					lockFundCache.recover(context.Height);
					// Unlock amounts
					for(auto& irecord : record->LockFundRecords)
					{
						auto account = accountStateCache.find(irecord.first).get();
						if(irecord.second.Active()) //Redundant
						{
							for(auto& mosaic : irecord.second.Get())
							{
								account.Balances.lock(mosaic.first, mosaic.second);
							}
						}
					}
				}
			}
		}
	}
	/// Note: Consider removing records in batches instead
	DEFINE_OBSERVER(LockFundBlock, model::BlockNotification<1>, ProcessLockfundUpdate);
}}
