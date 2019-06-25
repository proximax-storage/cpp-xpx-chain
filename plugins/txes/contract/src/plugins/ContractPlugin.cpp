/**
*** Copyright (c) 2018-present,
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

#include "catapult/plugins/PluginManager.h"
#include "ContractPlugin.h"
#include "src/cache/ContractCache.h"
#include "src/cache/ContractCacheStorage.h"
#include "src/cache/ReputationCache.h"
#include "src/cache/ReputationCacheStorage.h"
#include "src/config/ContractConfiguration.h"
#include "src/observers/Observers.h"
#include "src/plugins/ModifyContractTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	void RegisterContractSubsystem(PluginManager& manager) {
		manager.addTransactionSupport(CreateModifyContractTransactionPlugin());
		auto config = model::LoadPluginConfiguration<config::ContractConfiguration>(manager.config(), PLUGIN_NAME(contract));

		manager.addCacheSupport<cache::ContractCacheStorage>(
			std::make_unique<cache::ContractCache>(manager.cacheConfig(cache::ContractCache::Name)));

		manager.addCacheSupport<cache::ReputationCacheStorage>(
			std::make_unique<cache::ReputationCache>(manager.cacheConfig(cache::ReputationCache::Name)));

		using CacheHandlersContract = CacheHandlers<cache::ContractCacheDescriptor>;
		CacheHandlersContract::Register<model::FacilityCode::Contract>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("CONTRACT C"), [&cache]() {
				return cache.sub<cache::ContractCache>().createView()->size();
			});
		});

		using CacheHandlersReputation = CacheHandlers<cache::ReputationCacheDescriptor>;
		CacheHandlersReputation::Register<model::FacilityCode::Reputation>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("REPUTATION C"), [&cache]() {
				return cache.sub<cache::ReputationCache>().createView()->size();
			});
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
					.add(validators::CreateModifyContractCustomersValidator())
					.add(validators::CreateModifyContractExecutorsValidator())
					.add(validators::CreateModifyContractVerifiersValidator());
		});

		manager.addStatefulValidatorHook([](auto& builder) {
			builder
					.add(validators::CreateModifyContractInvalidCustomersValidator())
					.add(validators::CreateModifyContractInvalidExecutorsValidator())
					.add(validators::CreateModifyContractInvalidVerifiersValidator())
					.add(validators::CreateModifyContractDurationValidator());
		});

		manager.addObserverHook([config](auto& builder) {
			builder.add(observers::CreateModifyContractObserver(config));
			builder.add(observers::CreateReputationUpdateObserver());
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterContractSubsystem(manager);
}
