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

#include "CoreSystem.h"
#include "handlers/CoreDiagnosticHandlers.h"
#include "observers/Observers.h"
#include "validators/Validators.h"
#include "catapult/cache_core/AccountStateCacheStorage.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "catapult/observers/ObserverUtils.h"
#include "catapult/plugins/PluginManager.h"

namespace catapult { namespace plugins {

	namespace {
		cache::AccountStateCacheTypes::Options CreateAccountStateCacheOptions(const model::BlockChainConfiguration& config) {
			return { config.Network.Identifier, config.MinHarvesterBalance, config.EffectiveBalanceRange };
		}

		void AddAccountStateCache(PluginManager& manager, const model::BlockChainConfiguration& config) {
			using namespace catapult::cache;

			auto cacheConfig = manager.cacheConfig(AccountStateCache::Name);
			auto cacheOptions = CreateAccountStateCacheOptions(config);
			manager.addCacheSupport<AccountStateCacheStorage>(std::make_unique<AccountStateCache>(cacheConfig, cacheOptions));

			manager.addDiagnosticHandlerHook([](auto& handlers, const CatapultCache& cache) {
				handlers::RegisterAccountInfosHandler(
						handlers,
						handlers::CreateAccountInfosProducerFactory(cache.sub<AccountStateCache>()));
			});

			manager.addDiagnosticCounterHook([](auto& counters, const CatapultCache& cache) {
				counters.emplace_back(utils::DiagnosticCounterId("ACNTST C"), [&cache]() {
					return cache.sub<AccountStateCache>().createView()->size();
				});
			});
		}
	}

	void RegisterCoreSystem(PluginManager& manager) {
		const auto& config = manager.config();

		AddAccountStateCache(manager, config);

		manager.addStatelessValidatorHook([&config](auto& builder) {
			builder
				.add(validators::CreateMaxTransactionsValidator(config.MaxTransactionsPerBlock))
				.add(validators::CreateAddressValidator(config.Network.Identifier))
				.add(validators::CreateNetworkValidator(config.Network.Identifier));
		});

		manager.addStatefulValidatorHook([&config](auto& builder) {
			builder
				.add(validators::CreateDeadlineValidator(config.MaxTransactionLifetime))
				.add(validators::CreateNemesisSinkValidator())
				.add(validators::CreateEligibleHarvesterValidator(config.MinHarvesterBalance))
				.add(validators::CreateBalanceReserveValidator())
				.add(validators::CreateBalanceTransferValidator());
		});

		manager.addObserverHook([&config](auto& builder) {
			builder
				.add(observers::CreateAccountAddressObserver())
				.add(observers::CreateAccountPublicKeyObserver())
				.add(observers::CreateBalanceObserver())
				.add(observers::CreateHarvestFeeObserver())
				.add(observers::CreateSnapshotCleanUpObserver(config));
		});
	}
}}
