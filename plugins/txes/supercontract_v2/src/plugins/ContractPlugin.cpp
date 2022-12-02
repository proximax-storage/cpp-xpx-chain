/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ContractPlugin.h"
#include "src/cache/SuperContractCacheStorage.h"
#include "src/cache/DriveContractCacheStorage.h"
#include "src/plugins/DeployContractTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "src/observers/Observers.h"
#include "catapult/plugins/CacheHandlers.h"
#include <catapult/model/SupercontractNotifications.h>

namespace catapult { namespace plugins {

	namespace {
		template<typename TUnresolvedData>
		const TUnresolvedData* castToUnresolvedData(const UnresolvedAmountData* pData) {
			if (!pData)
				CATAPULT_THROW_RUNTIME_ERROR("unresolved amount data pointer is null")

			auto pCast = dynamic_cast<const TUnresolvedData*>(pData);
			if (!pCast)
				CATAPULT_THROW_RUNTIME_ERROR("unresolved amount data pointer is of unexpected type")

			return pCast;
		}
	}

	void RegisterContractSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::SuperContractConfiguration>();
		});

		const auto& pConfigHolder = manager.configHolder();
		const auto& immutableConfig = manager.immutableConfig();
		manager.addTransactionSupport(CreateDeployContractTransactionPlugin(immutableConfig));

		auto& pBrowser = manager.driveStateBrowser();

		manager.addAmountResolver([&](const auto& cache, const auto& unresolved, auto& resolved) {
			switch (unresolved.Type) {
			case UnresolvedAmountType::ExecutorWork: {
				const auto* pWork = castToUnresolvedData<model::ExecutorWork>(unresolved.DataPtr);

				const auto& contractCache = cache.template sub<cache::SuperContractCache>();
				auto contractIter = contractCache.find(pWork->ContractKey);
				const state::SuperContractEntry& contractEntry = contractIter.get();

				auto executorsNumber = pBrowser->getOrderedReplicatorsCount(cache, contractEntry.driveKey());

				resolved = Amount(pWork->AmountPerExecutor.unwrap() * executorsNumber);
				return true;
			}
			case UnresolvedAmountType::AutomaticExecutionWork: {
				const auto* pWork = castToUnresolvedData<model::AutomaticExecutorWork>(unresolved.DataPtr);

				const auto& contractCache = cache.template sub<cache::SuperContractCache>();
				auto contractIter = contractCache.find(pWork->ContractKey);
				const state::SuperContractEntry& contractEntry = contractIter.get();

				auto executorsNumber = pBrowser->getOrderedReplicatorsCount(cache, contractEntry.driveKey());

				resolved =
						Amount(pWork->NumberOfExecutions *
							   contractEntry.automaticExecutionsInfo().m_automatedExecutionCallPayment.unwrap() *
							   executorsNumber);
				return true;
			}
			case UnresolvedAmountType::AutomaticDownloadWork: {
				const auto* pWork = castToUnresolvedData<model::AutomaticExecutorWork>(unresolved.DataPtr);

				const auto& contractCache = cache.template sub<cache::SuperContractCache>();
				auto contractIter = contractCache.find(pWork->ContractKey);
				const state::SuperContractEntry& contractEntry = contractIter.get();

				auto executorsNumber = pBrowser->getOrderedReplicatorsCount(cache, contractEntry.driveKey());

				resolved =
						Amount(pWork->NumberOfExecutions *
						contractEntry.automaticExecutionsInfo().m_automatedDownloadCallPayment.unwrap() *
						executorsNumber);
				return true;
			}
		  	default:
			  	break;
			}

			return false;
		});

		manager.addCacheSupport<cache::SuperContractCacheStorage>(
				std::make_unique<cache::SuperContractCache>(manager.cacheConfig(cache::SuperContractCache::Name), pConfigHolder));

		using SuperContractCacheHandlersService = CacheHandlers<cache::SuperContractCacheDescriptor>;
		SuperContractCacheHandlersService::Register<model::FacilityCode::SuperContractCache>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("SUPER CONTRACT C"), [&cache]() {
				return cache.sub<cache::SuperContractCache>().createView(cache.height())->size();
			});
		});

		using DriveContractCacheHandlersService = CacheHandlers<cache::DriveContractCacheDescriptor>;
		DriveContractCacheHandlersService::Register<model::FacilityCode::DriveContractCache>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("DRIVE CONTRACT C"), [&cache]() {
				return cache.sub<cache::DriveContractCache>().createView(cache.height())->size();
			});
		});

		const auto& liquidityProviderValidator = manager.liquidityProviderExchangeValidator();
		const auto& liquidityProviderObserver = manager.liquidityProviderExchangeObserver();

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateSuperContractPluginConfigValidator());
		});

		manager.addStatefulValidatorHook([pConfigHolder, &immutableConfig](auto& builder) {
		  	builder
				.add(validators::CreateDeployContractValidator())
				.add(validators::CreateAutomaticExecutionsReplenishementValidator())
				.add(validators::CreateManualCallValidator());
		});

		manager.addObserverHook([&liquidityProviderObserver](auto& builder) {
			builder
				.add(observers::CreateDeployContractObserver())
				.add(observers::CreateAutomaticExecutionsReplenishmentObserver())
				.add(observers::CreateManualCallObserver());
		});
	}
}}


extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
    catapult::plugins::RegisterContractSubsystem(manager);
}
