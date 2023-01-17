/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ViewSequenceCacheDelta.h"
#include "ViewSequenceCacheView.h"
#include "catapult/cache/BasicCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

	/// Cache composed of view sequence information.
	using BasicViewHistoryCache = BasicCache<ViewSequenceCacheDescriptor,
			ViewSequenceCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of view sequence information.
	class ViewSequenceCache : public SynchronizedCache<BasicViewHistoryCache> {
	public:
		DEFINE_CACHE_CONSTANTS(ViewSequence)

	public:
		/// Creates a cache around \a config and \a pConfigHolder.
		explicit ViewSequenceCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: SynchronizedCache<BasicViewHistoryCache>(BasicViewHistoryCache(config, std::move(pConfigHolder)))
		{}
	};
}}
