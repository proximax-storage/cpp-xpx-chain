/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/CacheHandlers.h"
#include "CommitteePlugin.h"
#include "src/cache/CommitteeCacheSubCachePlugin.h"
#include "src/chain/WeightedVotingCommitteeManager.h"
#include "src/chain/WeightedVotingCommitteeManagerV2.h"
#include "src/chain/WeightedVotingCommitteeManagerV3.h"
#include "src/observers/Observers.h"
#include "src/plugins/AddHarvesterTransactionPlugin.h"
#include "src/plugins/RemoveHarvesterTransactionPlugin.h"
#include "src/validators/Validators.h"

namespace catapult { namespace plugins {

	void RegisterCommitteeSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::CommitteeConfiguration>();
		});

		manager.addTransactionSupport(CreateAddHarvesterTransactionPlugin());
		manager.addTransactionSupport(CreateRemoveHarvesterTransactionPlugin());

		auto pAccountCollector = std::make_shared<cache::CommitteeAccountCollector>();
		auto pConfigHolder = manager.configHolder();
		manager.addCacheSupport(std::make_unique<cache::CommitteeCacheSubCachePlugin>(manager.cacheConfig(cache::CommitteeCache::Name), pAccountCollector, pConfigHolder));

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("COMMITTEE C"), [&cache]() {
				return cache.sub<cache::CommitteeCache>().createView(cache.height())->size();
			});
		});

		auto pCommitteeManager = std::make_shared<chain::WeightedVotingCommitteeManager>(pAccountCollector);
		manager.setCommitteeManager(4, pCommitteeManager);
		auto pCommitteeManagerV2 = std::make_shared<chain::WeightedVotingCommitteeManagerV2>(pAccountCollector);
		manager.setCommitteeManager(5, pCommitteeManagerV2);
		auto pCommitteeManagerV3 = std::make_shared<chain::WeightedVotingCommitteeManagerV3>(pAccountCollector);
		manager.setCommitteeManager(6, pCommitteeManagerV3);

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateCommitteePluginConfigValidator());
		});

		manager.addStatefulValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateAddHarvesterValidator())
				.add(validators::CreateRemoveHarvesterValidator());
		});

		manager.addObserverHook([pCommitteeManager, pCommitteeManagerV2, pCommitteeManagerV3, pAccountCollector](auto& builder) {
			builder
				.add(observers::CreateAddHarvesterObserver())
				.add(observers::CreateRemoveHarvesterObserver())
				.add(observers::CreateUpdateHarvestersV1Observer(pCommitteeManager, pAccountCollector))
				.add(observers::CreateUpdateHarvestersV2Observer(pCommitteeManagerV2, pAccountCollector))
				.add(observers::CreateUpdateHarvestersV3Observer(pCommitteeManagerV3, pAccountCollector))
				.add(observers::CreateActiveHarvestersV1Observer())
				.add(observers::CreateActiveHarvestersV2Observer())
				.add(observers::CreateInactiveHarvestersObserver())
				.add(observers::CreateRemoveDbrbProcessByNetworkObserver(pAccountCollector));
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterCommitteeSubsystem(manager);
}
