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

#include "MultisigPlugin.h"
#include "src/cache/MultisigCache.h"
#include "src/observers/Observers.h"
#include "src/plugins/ModifyMultisigAccountTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	namespace {
		void extractCosigners(const cache::MultisigCache::CacheReadOnlyType& cache, const Key& publicKey, model::PublicKeySet& result) {
			if (!cache.contains(publicKey)) {
				return;
			}

			auto multisigIter = cache.find(publicKey);
			const auto& multisigEntry = multisigIter.get();

			for (const auto& cosignatoryPublicKey : multisigEntry.cosignatories()) {
				result.insert(cosignatoryPublicKey);
				extractCosigners(cache, cosignatoryPublicKey, result);
			}
		}
	}

	void RegisterMultisigSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::MultisigConfiguration>();
		});

		manager.addTransactionSupport(CreateModifyMultisigAccountTransactionPlugin());

		manager.addCacheSupport<cache::MultisigCacheStorage>(
				std::make_unique<cache::MultisigCache>(manager.cacheConfig(cache::MultisigCache::Name)));

		using CacheHandlers = CacheHandlers<cache::MultisigCacheDescriptor>;
		CacheHandlers::Register<model::FacilityCode::Multisig>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("MULTISIG C"), [&cache]() {
				return cache.sub<cache::MultisigCache>().createView(cache.height())->size();
			});
		});

		manager.addPublicKeysExtractor([](const auto& cache, const auto& key) {
			const auto& multisigCache = cache.template sub<cache::MultisigCache>();
			auto result = model::PublicKeySet{ key };
			extractCosigners(multisigCache, key, result);

			return result;
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateModifyMultisigCosignersValidator())
				.add(validators::CreateMultisigPluginConfigValidator());
		});

		manager.addStatefulValidatorHook([&transactionRegistry = manager.transactionRegistry()](auto& builder) {
			builder
				.add(validators::CreateMultisigPermittedOperationValidator())
				.add(validators::CreateModifyMultisigMaxCosignedAccountsValidator())
				.add(validators::CreateModifyMultisigMaxCosignersValidator())
				.add(validators::CreateModifyMultisigInvalidCosignersValidator())
				.add(validators::CreateModifyMultisigInvalidSettingsValidator())
				// notice that ModifyMultisigLoopAndLevelValidator must be called before multisig aggregate validators
				.add(validators::CreateModifyMultisigLoopAndLevelValidator())
				// notice that ineligible cosigners must dominate missing cosigners in order for cosigner aggregation to work
				.add(validators::CreateMultisigAggregateEligibleCosignersValidator(transactionRegistry))
				.add(validators::CreateMultisigAggregateSufficientCosignersValidator(transactionRegistry));
		});

		manager.addObserverHook([](auto& builder) {
			// notice that ModifyMultisigCosignersObserver must be called before ModifyMultisigSettingsObserver because
			// the ModifyMultisigSettingsObserver interprets a missing entry in the multisig cache for the notification signer
			// as conversion from a multisig to a normal account done by the ModifyMultisigCosignersObserver
			builder
				.add(observers::CreateModifyMultisigCosignersObserver())
				.add(observers::CreateModifyMultisigSettingsObserver());
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterMultisigSubsystem(manager);
}
