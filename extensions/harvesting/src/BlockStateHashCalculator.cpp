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

#include "BlockStateHashCalculator.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/chain/BlockExecutor.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/observers/NotificationObserverAdapter.h"
#include "catapult/plugins/PluginManager.h"

namespace catapult { namespace harvesting {

	std::pair<Hash256, bool> CalculateBlockStateHash(
			const model::BlockElement& blockElements,
			const cache::CatapultCache& cache,
			const model::BlockChainConfiguration& config,
			const plugins::PluginManager& pluginManager) {
		// 0. bypass calculation if disabled
		if (!config.ShouldEnableVerifiableState)
			return std::make_pair(Hash256(), true);

		// 1. lock the cache and make sure the height is consistent with the *next* block
		auto cacheView = cache.createView();
		if (cacheView.height() + Height(1) != blockElements.Block.Height) {
			CATAPULT_LOG(debug)
					<< "bypassing state hash calculation because cache height (" << cacheView.height()
					<< ") is inconsistent with block height (" << blockElements.Block.Height << ")";
			return std::make_pair(Hash256(), false);
		}

		// 2. prepare observer
		observers::NotificationObserverAdapter entityObserver(pluginManager.createObserver(), pluginManager.createNotificationPublisher());

		// 3. prepare observer state (for the *next* harvested block)
		const auto& accountStateCache = cache.sub<cache::AccountStateCache>();
		auto importanceHeight = model::ConvertToImportanceHeight(blockElements.Block.Height, accountStateCache.importanceGrouping());

		auto cacheDetachedDelta = cache.createDetachableDelta().detach();
		auto pCacheDelta = cacheDetachedDelta.lock();
		auto catapultState = state::CatapultState();
		catapultState.LastRecalculationHeight = importanceHeight;
		auto observerState = observers::ObserverState(*pCacheDelta, catapultState);

		// 4. execute block
		chain::ExecuteBlock(blockElements, entityObserver, observerState);
		return std::make_pair(pCacheDelta->calculateStateHash(blockElements.Block.Height).StateHash, true);
	}
}}
