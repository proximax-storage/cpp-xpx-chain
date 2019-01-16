/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "plugins/txes/multisig/src/cache/MultisigCache.h"
#include "plugins/txes/contract/src/cache/ContractCache.h"
#include "plugins/txes/contract/src/cache/ContractCacheStorage.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "tests/test/cache/CacheTestUtils.h"

namespace catapult { namespace test {

	/// Cache factory for creating a catapult cache composed of contract cache and core caches.
	struct ContractCacheFactory {
	private:
		static auto CreateSubCaches() {
			auto contractCacheId = cache::ContractCache::Id;
			auto multisigCacheId = cache::MultisigCache::Id;
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(std::max(contractCacheId, multisigCacheId) + 1);
			subCaches[contractCacheId] = MakeSubCachePlugin<cache::ContractCache, cache::ContractCacheStorage>();
			subCaches[multisigCacheId] = MakeSubCachePlugin<cache::MultisigCache, cache::MultisigCacheStorage>();
			return subCaches;
		}

	public:
		/// Creates an empty catapult cache around default configuration.
		static cache::CatapultCache Create() {
			return Create(model::BlockChainConfiguration::Uninitialized());
		}

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const model::BlockChainConfiguration& config) {
			auto subCaches = CreateSubCaches();
			CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
			return cache::CatapultCache(std::move(subCaches));
		}
	};
}}
