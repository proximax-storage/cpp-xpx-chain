/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DbrbViewCacheDelta.h"
#include "DbrbViewCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	using DbrbViewBasicCache = BasicCache<DbrbViewCacheDescriptor, DbrbViewCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Cache composed of network config information.
	class BasicDbrbViewCache : public DbrbViewBasicCache {
	public:
		/// Creates a cache.
		explicit BasicDbrbViewCache(
			const CacheConfiguration& config,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder,
			std::shared_ptr<DbrbViewFetcherImpl> pDbrbViewFetcher)
				: DbrbViewBasicCache(config, std::move(pConfigHolder))
				, m_pDbrbViewFetcher(std::move(pDbrbViewFetcher))
		{}

	public:
		/// Initializes the cache with \a heights.
		void init(const std::set<dbrb::ProcessId>& processIds) {
			auto delta = createDelta(Height());
			for (const auto& processId : processIds)
				delta.insertProcessId(processId);
			commit(delta);
		}

		void commit(const CacheDeltaType& delta) {
			delta.updateDbrbView(*m_pDbrbViewFetcher);
			DbrbViewBasicCache::commit(delta);
		}

	private:
		std::shared_ptr<DbrbViewFetcherImpl> m_pDbrbViewFetcher;
	};

	/// Synchronized cache composed of DBRB view information.
	class DbrbViewCache : public SynchronizedCacheWithInit<BasicDbrbViewCache> {
	public:
		DEFINE_CACHE_CONSTANTS(DbrbView)

	public:
		/// Creates a cache around \a config and \a pConfigHolder.
		explicit DbrbViewCache(
				const CacheConfiguration& config,
				std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder,
				std::shared_ptr<DbrbViewFetcherImpl> pDbrbViewFetcher)
			: SynchronizedCacheWithInit<BasicDbrbViewCache>(BasicDbrbViewCache(config, std::move(pConfigHolder), std::move(pDbrbViewFetcher)))
		{}
	};
}}
