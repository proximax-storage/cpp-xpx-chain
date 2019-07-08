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

#include "NamespacePlugin.h"
#include "AddressAliasTransactionPlugin.h"
#include "MosaicAliasTransactionPlugin.h"
#include "RegisterNamespaceTransactionPlugin.h"
#include "src/cache/NamespaceCache.h"
#include "src/cache/NamespaceCacheStorage.h"
#include "src/cache/NamespaceCacheSubCachePlugin.h"
#include "src/model/NamespaceReceiptType.h"
#include "src/observers/Observers.h"
#include "src/validators/Validators.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"
#include "catapult/observers/ObserverUtils.h"
#include "catapult/observers/RentalFeeObserver.h"
#include "catapult/plugins/CacheHandlers.h"
#include "catapult/plugins/PluginManager.h"

namespace catapult { namespace plugins {

	namespace {
		// region alias

		void RegisterAliasSubsystem(PluginManager& manager) {
			manager.addTransactionSupport(CreateAddressAliasTransactionPlugin());
			manager.addTransactionSupport(CreateMosaicAliasTransactionPlugin());

			manager.addStatelessValidatorHook([](auto& builder) {
				builder.add(validators::CreateAliasActionValidator());
			});

			manager.addStatefulValidatorHook([](auto& builder) {
				builder
					.add(validators::CreateAliasAvailabilityValidator())
					.add(validators::CreateUnlinkAliasedAddressConsistencyValidator())
					.add(validators::CreateUnlinkAliasedMosaicIdConsistencyValidator())
					.add(validators::CreateAddressAliasValidator());
			});

			manager.addObserverHook([](auto& builder) {
				builder
					.add(observers::CreateAliasedAddressObserver())
					.add(observers::CreateAliasedMosaicIdObserver());
			});
		}

		// endregion

		// region namespace

		template<typename TAliasValue, typename TAliasValueAccessor>
		bool RunNamespaceResolver(
				const cache::NamespaceCacheTypes::CacheReadOnlyType& namespaceCache,
				NamespaceId namespaceId,
				state::AliasType aliasType,
				TAliasValue& aliasValue,
				TAliasValueAccessor aliasValueAccessor) {
			auto iter = namespaceCache.find(namespaceId);
			if (!iter.tryGet())
				return false;

			const auto& alias = iter.get().root().alias(namespaceId);
			if (aliasType != alias.type())
				return false;

			aliasValue = aliasValueAccessor(alias);
			return true;
		}

		void RegisterNamespaceAliasResolvers(PluginManager& manager) {
			manager.addMosaicResolver([](const auto&, const auto& unresolved, auto& resolved) {
				constexpr uint64_t Namespace_Flag = 1ull << 63;
				if (0 == (Namespace_Flag & unresolved.unwrap())) {
					resolved = model::ResolverContext().resolve(unresolved);
					return true;
				}

				return false;
			});

			manager.addMosaicResolver([](const auto& cache, const auto& unresolved, auto& resolved) {
				auto namespaceCache = cache.template sub<cache::NamespaceCache>();
				auto namespaceId = NamespaceId(unresolved.unwrap());
				return RunNamespaceResolver(namespaceCache, namespaceId, state::AliasType::Mosaic, resolved, [](const auto& alias) {
					return alias.mosaicId();
				});
			});

			manager.addAddressResolver([](const auto&, const auto& unresolved, auto& resolved) {
				if (0 == (1 & unresolved[0].Byte)) {
					resolved = model::ResolverContext().resolve(unresolved);
					return true;
				}

				return false;
			});

			manager.addAddressResolver([](const auto& cache, const auto& unresolved, auto& resolved) {
				auto namespaceCache = cache.template sub<cache::NamespaceCache>();
				NamespaceId namespaceId;
				std::memcpy(static_cast<void*>(&namespaceId), unresolved.data() + 1, sizeof(NamespaceId));
				return RunNamespaceResolver(namespaceCache, namespaceId, state::AliasType::Address, resolved, [](const auto& alias) {
					return alias.address();
				});
			});
		}

		auto GetNamespaceView(const cache::CatapultCache& cache) {
			return cache.sub<cache::NamespaceCache>().createView(cache.height());
		}

		void RegisterNamespaceSubsystemOnly(PluginManager& manager) {
			const auto& pConfigHolder = manager.configHolder();
			manager.addTransactionSupport(CreateRegisterNamespaceTransactionPlugin(pConfigHolder));

			RegisterNamespaceAliasResolvers(manager);
			manager.addCacheSupport(std::make_unique<cache::NamespaceCacheSubCachePlugin>(
					manager.cacheConfig(cache::NamespaceCache::Name),
					cache::NamespaceCacheTypes::Options{ pConfigHolder }));

			using CacheHandlers = CacheHandlers<cache::NamespaceCacheDescriptor>;
			CacheHandlers::Register<model::FacilityCode::Namespace>(manager);

			manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
				counters.emplace_back(utils::DiagnosticCounterId("NS C"), [&cache]() { return GetNamespaceView(cache)->size(); });
				counters.emplace_back(utils::DiagnosticCounterId("NS C AS"), [&cache]() { return GetNamespaceView(cache)->activeSize(); });
				counters.emplace_back(utils::DiagnosticCounterId("NS C DS"), [&cache]() { return GetNamespaceView(cache)->deepSize(); });
			});

			manager.addStatelessValidatorHook([](auto& builder) {
				builder
					.add(validators::CreatePluginConfigValidator())
					.add(validators::CreateNamespaceTypeValidator());
			});

			manager.addStatefulValidatorHook([&pConfigHolder](auto& builder) {
				builder
					.add(validators::CreateNamespaceNameValidator(pConfigHolder))
					.add(validators::CreateRootNamespaceValidator(pConfigHolder))
					.add(validators::CreateRootNamespaceAvailabilityValidator(pConfigHolder))
					// note that the following validator needs to run before the RootNamespaceMaxChildrenValidator
					.add(validators::CreateChildNamespaceAvailabilityValidator())
					.add(validators::CreateRootNamespaceMaxChildrenValidator(pConfigHolder));
			});

			manager.addObserverHook([&pConfigHolder](auto& builder) {
				auto rentalFeeReceiptType = model::Receipt_Type_Namespace_Rental_Fee;
				auto expiryReceiptType = model::Receipt_Type_Namespace_Expired;
				builder
					.add(observers::CreateRootNamespaceObserver())
					.add(observers::CreateChildNamespaceObserver())
					.add(observers::CreateRentalFeeObserver<model::NamespaceRentalFeeNotification<1>>("Namespace", rentalFeeReceiptType))
					.add(observers::CreateCacheBlockTouchObserver<cache::NamespaceCache>("Namespace", expiryReceiptType))
					.add(observers::CreateCacheBlockPruningObserver<cache::NamespaceCache>("Namespace", 1, pConfigHolder));
			});
		}

		// endregion
	}

	void RegisterNamespaceSubsystem(PluginManager& manager) {
		RegisterNamespaceSubsystemOnly(manager);
		RegisterAliasSubsystem(manager);
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterNamespaceSubsystem(manager);
}
