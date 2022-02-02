/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "src/cache/LockFundCache.h"
#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"


namespace catapult { namespace observers {

	DEFINE_OBSERVER(LockFundBlock, model::BlockNotification<1>, ([](const auto& notification, const ObserverContext& context) {
		auto& cache = context.Cache.sub<cache::AccountStateCache>();
		auto& lockFundCache = context.Cache.sub<cache::LockFundCache>();
		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto heightRecord = lockFundCache.find(context.Height);
		auto records = heightRecord.tryGet();
		if(context.Mode == NotifyMode::Commit)
		{
			if(records)
			{
				lockFundCache.template actAndToggle(context.Height, false, [&accountStateCache, &context](const Key& publicKey, const std::map<MosaicId, Amount>& mosaics)
				{
				  auto accountState = accountStateCache.find(publicKey).get();
				  for(auto& mosaic : mosaics)
				  {
					  accountState.Balances.unlock(mosaic.first, mosaic.second, context.Height);
				  }
				});
			}
		}
		else
		{
			lockFundCache.template actAndToggle(context.Height, true, [&accountStateCache, &context](const Key& publicKey, const std::map<MosaicId, Amount>& mosaics)
			{
			  auto accountState = accountStateCache.find(publicKey).get();
			  for(auto& mosaic : mosaics)
			  {
				  accountState.Balances.lock(mosaic.first, mosaic.second, context.Height);
			  }
			});
		}
	}));
}}
