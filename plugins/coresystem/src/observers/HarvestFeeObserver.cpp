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

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(HarvestFee, model::BlockNotification, [](const auto& notification, const ObserverContext& context) {
		// credit the harvester
		auto& cache = context.Cache.sub<cache::AccountStateCache>();
		auto accountStateIter = cache.find(notification.Signer);
		auto& harvesterState = accountStateIter.get();
		if (NotifyMode::Commit == context.Mode)
			harvesterState.Balances.credit(Xpx_Id, notification.TotalFee, context.Height);
		else
			harvesterState.Balances.debit(Xpx_Id, notification.TotalFee, context.Height);
	});
}}
