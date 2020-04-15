/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/PluginManager.h"
#include "SuperContractPlugin.h"
#include "src/cache/SuperContractCache.h"
#include "src/cache/SuperContractCacheStorage.h"
#include "src/observers/Observers.h"
#include "src/plugins/DeployTransactionPlugin.h"
#include "src/plugins/StartExecuteTransactionPlugin.h"
#include "src/plugins/EndExecuteTransactionPlugin.h"
#include "src/plugins/UploadFileTransactionPlugin.h"
#include "src/plugins/DeactivateTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	void RegisterSuperContractSubsystem(PluginManager& manager) {

		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::SuperContractConfiguration>();
		});

        const auto& pConfigHolder = manager.configHolder();
        manager.addTransactionSupport(CreateDeployTransactionPlugin());
        manager.addTransactionSupport(CreateStartExecuteTransactionPlugin(manager.configHolder()));
        manager.addTransactionSupport(CreateEndExecuteTransactionPlugin());
        manager.addTransactionSupport(CreateUploadFileTransactionPlugin());
        manager.addTransactionSupport(CreateDeactivateTransactionPlugin());

		manager.addCacheSupport<cache::SuperContractCacheStorage>(
			std::make_unique<cache::SuperContractCache>(manager.cacheConfig(cache::SuperContractCache::Name), pConfigHolder));

		using SuperContractCacheHandlersSuperContract = CacheHandlers<cache::SuperContractCacheDescriptor>;
		SuperContractCacheHandlersSuperContract::Register<model::FacilityCode::SuperContract>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("SC C"), [&cache]() {
				return cache.sub<cache::SuperContractCache>().createView(cache.height())->size();
			});
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateEmbeddedTransactionValidator())
				.add(validators::CreateSuperContractPluginConfigValidator());
		});

		manager.addStatefulValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateStartExecuteValidator())
				.add(validators::CreateEndExecuteValidator())
				.add(validators::CreateSuperContractValidator())
				.add(validators::CreateDeployValidator())
				.add(validators::CreateDriveFileSystemValidator())
				.add(validators::CreateEndOperationValidator())
				.add(validators::CreateDeactivateValidator());
		});

		manager.addObserverHook([](auto& builder) {
			builder
				.add(observers::CreateDeployObserver())
				.add(observers::CreateStartExecuteObserver())
				.add(observers::CreateEndExecuteCosignersObserver())
				.add(observers::CreateEndExecuteObserver())
				.add(observers::CreateExpiredExecutionObserver())
				.add(observers::CreateAggregateTransactionHashObserver())
				.add(observers::CreateDeactivateObserver())
				.add(observers::CreateEndDriveObserver());
		});
	}
}}


extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
    catapult::plugins::RegisterSuperContractSubsystem(manager);
}
