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

	/// Note: Consider removing records in batches instead
	DEFINE_OBSERVER(LockFundBlock, model::BlockNotification<1>, ([](const auto& notification, const ObserverContext& context) {
		if (context.Mode == NotifyMode::Rollback)
			return;
		const model::NetworkConfiguration& config = context.Config.Network;
	  	if (context.Height.unwrap() - config.MaxRollbackBlocks <= 0)
			return;

		auto targetHeight = context.Height-Height(config.MaxRollbackBlocks);
		auto& lockFundCache = context.Cache.sub<cache::LockFundCache>();
		auto heightRecord = lockFundCache.find(targetHeight);
		auto records = heightRecord.tryGet();
		if(records)
		{
			lockFundCache.prune(targetHeight);
		}
	}));
}}
