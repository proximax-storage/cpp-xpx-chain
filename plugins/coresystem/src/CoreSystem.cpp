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
#include "catapult/cache_core/AccountStateCacheSubCachePlugin.h"
#include "catapult/cache_core/BlockDifficultyCacheSubCachePlugin.h"
#include "catapult/observers/ObserverUtils.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	namespace {
		cache::AccountStateCacheTypes::Options CreateAccountStateCacheOptions(PluginManager& manager) {
			const auto& pConfigHolder = manager.configHolder();
			const auto& config = pConfigHolder->Config().Immutable;
			return {
				pConfigHolder,
				config.NetworkIdentifier,
				config.CurrencyMosaicId,
				config.HarvestingMosaicId
			};
		}

		void AddAccountStateCache(PluginManager& manager) {
			using namespace catapult::cache;

			auto cacheConfig = manager.cacheConfig(AccountStateCache::Name);
			auto cacheOptions = CreateAccountStateCacheOptions(manager);
			manager.addCacheSupport(std::make_unique<AccountStateCacheSubCachePlugin>(cacheConfig, cacheOptions));

			using CacheHandlers = CacheHandlers<cache::AccountStateCacheDescriptor>;
			CacheHandlers::Register<model::FacilityCode::Core>(manager);

			manager.addDiagnosticCounterHook([](auto& counters, const CatapultCache& cache) {
				counters.emplace_back(utils::DiagnosticCounterId("ACNTST C"), [&cache]() {
					return cache.sub<AccountStateCache>().createView(cache.height())->size();
				});
				counters.emplace_back(utils::DiagnosticCounterId("ACNTST C HVA"), [&cache]() {
					return cache.sub<AccountStateCache>().createView(cache.height())->highValueAddresses().size();
				});
			});
		}

		void AddBlockDifficultyCache(PluginManager& manager) {
			using namespace catapult::cache;

			manager.addCacheSupport(std::make_unique<BlockDifficultyCacheSubCachePlugin>(manager.configHolder()));

			manager.addDiagnosticCounterHook([](auto& counters, const CatapultCache& cache) {
				counters.emplace_back(utils::DiagnosticCounterId("BLKDIF C"), [&cache]() {
					return cache.sub<BlockDifficultyCache>().createView(cache.height())->size();
				});
			});
		}
	}

	void RegisterCoreSystem(PluginManager& manager) {
		const auto& pConfigHolder = manager.configHolder();

		AddAccountStateCache(manager);
		AddBlockDifficultyCache(manager);

		auto networkIdentifier = pConfigHolder->Config().Immutable.NetworkIdentifier;
		manager.addStatelessValidatorHook([networkIdentifier](auto& builder) {
			builder
				.add(validators::CreateNetworkValidator(networkIdentifier))
				.add(validators::CreateTransactionFeeValidator());
		});

		manager.addStatefulValidatorHook([networkIdentifier](auto& builder) {
			builder
				.add(validators::CreateEntityVersionValidator())
				.add(validators::CreateMaxTransactionsValidator())
				.add(validators::CreateAddressValidator(networkIdentifier))
				.add(validators::CreateDeadlineValidator())
//				We using nemesis account to update the network
//				.add(validators::CreateNemesisSinkValidator())
				.add(validators::CreateEligibleHarvesterValidator())
				.add(validators::CreateBalanceDebitValidator())
				.add(validators::CreateBalanceTransferValidator())
				.add(validators::CreateLevyBalanceValidator());
		});

		manager.addObserverHook([pConfigHolder](auto& builder) {
			builder
				.add(observers::CreateSourceChangeObserver())
				.add(observers::CreateAccountAddressObserver())
				.add(observers::CreateAccountPublicKeyObserver())
				.add(observers::CreateBalanceDebitObserver())
				.add(observers::CreateBalanceCreditObserver())
				.add(observers::CreateBalanceTransferObserver())
				.add(observers::CreateLevyTransferObserver())
				.add(observers::CreateHarvestFeeObserver(pConfigHolder))
				.add(observers::CreateTotalTransactionsObserver())
				.add(observers::CreateSnapshotCleanUpObserver());
		});

		manager.addTransientObserverHook([](auto& builder) {
			builder
				.add(observers::CreateBlockDifficultyObserver())
				.add(observers::CreateCacheBlockPruningObserver<cache::BlockDifficultyCache>("BlockDifficulty"));
		});
	}
}}
