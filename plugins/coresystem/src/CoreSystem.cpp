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
#include "observers/Observers.h"
#include "validators/Validators.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/AccountStateCacheStorage.h"
#include "catapult/cache_core/AccountStateCacheSubCachePlugin.h"
#include "catapult/cache_core/BlockDifficultyCacheStorage.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "catapult/observers/ObserverUtils.h"
#include "catapult/plugins/CacheHandlers.h"
#include "catapult/plugins/PluginManager.h"

namespace catapult { namespace plugins {

	namespace {
		void AddAccountStateCache(PluginManager& manager) {
			using namespace catapult::cache;

			auto cacheConfig = manager.cacheConfig(AccountStateCache::Name);
			manager.addCacheSupport(std::make_unique<AccountStateCacheSubCachePlugin>(cacheConfig, manager.config()));

			using CacheHandlers = CacheHandlers<cache::AccountStateCacheDescriptor>;
			CacheHandlers::Register<model::FacilityCode::Core>(manager);

			manager.addDiagnosticCounterHook([](auto& counters, const CatapultCache& cache) {
				counters.emplace_back(utils::DiagnosticCounterId("ACNTST C"), [&cache]() {
					return cache.sub<AccountStateCache>().createView()->size();
				});
				counters.emplace_back(utils::DiagnosticCounterId("ACNTST C HVA"), [&cache]() {
					return cache.sub<AccountStateCache>().createView()->highValueAddresses().size();
				});
			});
		}

		void AddBlockDifficultyCache(PluginManager& manager) {
			using namespace catapult::cache;

			manager.addCacheSupport<BlockDifficultyCacheStorage>(std::make_unique<BlockDifficultyCache>(manager.config()));

			manager.addDiagnosticCounterHook([](auto& counters, const CatapultCache& cache) {
				counters.emplace_back(utils::DiagnosticCounterId("BLKDIF C"), [&cache]() {
					return cache.sub<BlockDifficultyCache>().createView()->size();
				});
			});
		}
	}

	void RegisterCoreSystem(PluginManager& manager) {
		const auto& config = manager.config();

		AddAccountStateCache(manager);
		AddBlockDifficultyCache(manager);

		manager.addStatelessValidatorHook([&config, &pConfigHolder = manager.configHolder()](auto& builder) {
			builder
				.add(validators::CreateMaxTransactionsValidator(config))
				.add(validators::CreateNetworkValidator(config))
				.add(validators::CreateEntityVersionValidator(pConfigHolder))
				.add(validators::CreateTransactionFeeValidator());
		});

		manager.addStatefulValidatorHook([&config](auto& builder) {
			builder
				.add(validators::CreateAddressValidator(config))
				.add(validators::CreateDeadlineValidator(config))
//				We using nemesis account to update the network
//				.add(validators::CreateNemesisSinkValidator())
				.add(validators::CreateEligibleHarvesterValidator(config))
				.add(validators::CreateBalanceDebitValidator())
				.add(validators::CreateBalanceTransferValidator());
		});

		manager.addObserverHook([&config](auto& builder) {
			builder
				.add(observers::CreateSourceChangeObserver())
				.add(observers::CreateAccountAddressObserver())
				.add(observers::CreateAccountPublicKeyObserver())
				.add(observers::CreateBalanceDebitObserver())
				.add(observers::CreateBalanceTransferObserver())
				.add(observers::CreateHarvestFeeObserver(config))
				.add(observers::CreateTotalTransactionsObserver())
				.add(observers::CreateSnapshotCleanUpObserver(config));
		});

		manager.addTransientObserverHook([&config](auto& builder) {
			builder
				.add(observers::CreateBlockDifficultyObserver())
				.add(observers::CreateCacheBlockPruningObserver<cache::BlockDifficultyCache>(
						"BlockDifficulty",
						config));
		});
	}
}}
