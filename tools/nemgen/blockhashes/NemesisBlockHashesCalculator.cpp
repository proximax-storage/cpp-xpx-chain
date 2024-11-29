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

#include "NemesisBlockHashesCalculator.h"
#include "PluginLoader.h"
#include "catapult/chain/BlockExecutor.h"
#include "catapult/observers/NotificationObserverAdapter.h"
#include "NemesisExecutionHasher.h"

namespace catapult { namespace tools { namespace nemgen {

	BlockExecutionHashesInfo CalculateNemesisBlockExecutionHashes(
			const NemesisConfiguration& nemesisConfig,
			const model::BlockElement& blockElement,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			NemesisTransactions* transactions,
			plugins::PluginManager* manager) {
		// 1. load all plugins
		PluginLoader pluginLoader(pConfigHolder);
		pluginLoader.loadAll();
		plugins::PluginManager* pluginManager;
		if(manager != nullptr)
			pluginManager = manager;
		else {
			pluginManager = &pluginLoader.manager();
		}

		auto initializers = pluginManager->createPluginInitializer();
		initializers(const_cast<model::NetworkConfiguration&>(pConfigHolder->Config().Network));

		// 2. prepare observer
		observers::NotificationObserverAdapter entityObserver(pluginManager->createObserver(), pluginManager->createNotificationPublisher());

		// 3. prepare observer state
		auto cache = pluginManager->createCache();
		pConfigHolder->SetCache(&cache);
		auto cacheDetachableDelta = cache.createDetachableDelta();
		auto cacheDetachedDelta = cacheDetachableDelta.detach();
		auto pCacheDelta = cacheDetachedDelta.tryLock();
		auto catapultState = state::CatapultState();
		auto blockStatementBuilder = model::BlockStatementBuilder();
		auto observerState = observers::ObserverState(*pCacheDelta, catapultState, blockStatementBuilder);

		// 4. prepare resolvers
		auto readOnlyCache = pCacheDelta->toReadOnly();
		auto resolverContext = pluginManager->createResolverContext(readOnlyCache);

		// 5. execute block
		if(nemesisConfig.EnableSpool)
		{
			auto transactionView = transactions->createView();
			ExecuteBlock(blockElement, { entityObserver, resolverContext, pConfigHolder, observerState }, transactionView, [&blockElement](const auto& transaction){
				return model::WeakEntityInfo(*transaction.transaction, transaction.entityHash, blockElement.Block);
			});
		}
		else chain::ExecuteBlock(blockElement, { entityObserver, resolverContext, pConfigHolder, observerState });

		auto cacheStateHashInfo = pCacheDelta->calculateStateHash(blockElement.Block.Height);
		auto blockReceiptsHash = pluginManager->immutableConfig().ShouldEnableVerifiableReceipts
				? model::CalculateMerkleHash(*blockStatementBuilder.build())
				: Hash256();

		// 6. Clear plugin configs before unloading modules.
		pConfigHolder->Config().Network.ClearPluginConfigurations();

		return { blockReceiptsHash, cacheStateHashInfo.StateHash, cacheStateHashInfo.SubCacheMerkleRoots };
	}
}}}
