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

	using Notification = model::BlockNotification<1>;

	DECLARE_OBSERVER(SnapshotCleanUp, Notification)(const model::BlockChainConfiguration& config) {
		return MAKE_OBSERVER(SnapshotCleanUp, Notification, [&config](const auto&, const ObserverContext& context) {
			if (context.Mode == NotifyMode::Rollback)
				return;

			if (config.MaxRollbackBlocks != 0 && context.Height.unwrap() % config.MaxRollbackBlocks)
				return;

			auto& cache = context.Cache.sub<cache::AccountStateCache>();
			auto updatedAddresses = cache.updatedAddresses();

			for (const auto& address : updatedAddresses) {
				auto pAccountState = cache.find(address).tryGet();

				if (!pAccountState)
					continue;

				pAccountState->Balances.maybeCleanUpSnapshots(context.Height, config);
			}
		});
	}
}}
