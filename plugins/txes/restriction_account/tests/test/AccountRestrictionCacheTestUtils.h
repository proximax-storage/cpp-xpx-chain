/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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

#pragma once
#include "plugins/services/globalstore/src/cache/GlobalStoreCacheStorage.h"
#include "plugins/services/globalstore/src/cache/GlobalStoreCache.h"
#include "plugins/txes/property/tests/test/PropertyCacheTestUtils.h"
#include "AccountRestrictionTestTraits.h"
#include "src/cache/AccountRestrictionCache.h"
#include "src/cache/AccountRestrictionCacheStorage.h"
#include "catapult/model/Address.h"
#include "AccountRestrictionTestUtils.h"
#include "tests/test/cache/CacheTestUtils.h"

namespace catapult { namespace test {

	/// Cache factory for creating a catapult cache composed of account restriction cache and core caches.
	struct AccountRestrictionCacheFactory {
	private:
		static auto CreateSubCachesWithAccountRestrictionCache(model::NetworkIdentifier networkIdentifier, uint maxAccountRestrictionValues) {
			auto cacheId_Temp = (cache::AccountRestrictionCache::Id > cache::PropertyCache::Id ? cache::AccountRestrictionCache::Id : cache::PropertyCache::Id);
			auto cacheId = cacheId_Temp > cache::GlobalStoreCache::Id ? cacheId_Temp : cache::GlobalStoreCache::Id;
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cacheId + 1);
			auto holder = CreateAccountRestrictionConfigHolder(maxAccountRestrictionValues, networkIdentifier);
			subCaches[cache::AccountRestrictionCache::Id] = MakeSubCachePlugin<cache::AccountRestrictionCache, cache::AccountRestrictionCacheStorage>(
					holder);
			subCaches[cache::PropertyCache::Id] = MakeSubCachePlugin<cache::PropertyCache, cache::PropertyCacheStorage>(holder);
			subCaches[cache::GlobalStoreCache::Id] = MakeSubCachePlugin<cache::GlobalStoreCache, cache::GlobalStoreCacheStorage>(holder);
			CoreSystemCacheFactory::AppendSubCaches(holder, subCaches);
			return subCaches;
		}

	public:
		/// Creates an empty catapult cache around default configuration.
		static cache::CatapultCache Create(uint maxAccountRestrictionValues = 10) {
			return Create(config::BlockchainConfiguration::Uninitialized(), maxAccountRestrictionValues);
		}

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const config::BlockchainConfiguration& config, uint maxAccountRestrictionValues = 10) {
			auto subCaches = CreateSubCachesWithAccountRestrictionCache(config.Immutable.NetworkIdentifier, maxAccountRestrictionValues);
			return cache::CatapultCache(std::move(subCaches));
		}
	};

	// region PopulateCache

	/// Populates \a delta with \a address and \a values.
	template<typename TRestrictionValueTraits, typename TOperationTraits = AllowTraits>
	void PopulateAccountRestrictionCache(
			cache::CatapultCacheDelta& delta,
			const Address& address,
			const std::vector<typename TRestrictionValueTraits::ValueType>& values) {
		auto& restrictionCacheDelta = delta.sub<cache::AccountRestrictionCache>();
		restrictionCacheDelta.insert(state::AccountRestrictions(address));
		auto& restrictions = restrictionCacheDelta.find(address).get();
		auto& restriction = restrictions.restriction(TRestrictionValueTraits::Restriction_Flags);
		for (const auto& value : values)
			TOperationTraits::Add(restriction, utils::ToVector(value));
	}

	/// Populates \a cache with \a address and \a values.
	template<typename TRestrictionValueTraits, typename TOperationTraits = AllowTraits>
	void PopulateAccountRestrictionCache(
			cache::CatapultCache& cache,
			const Address& address,
			const std::vector<typename TRestrictionValueTraits::ValueType>& values) {
		auto delta = cache.createDelta();
		PopulateAccountRestrictionCache<TRestrictionValueTraits, TOperationTraits>(delta, address, values);
		cache.commit(Height(1));
	}

	// endregion
}}
