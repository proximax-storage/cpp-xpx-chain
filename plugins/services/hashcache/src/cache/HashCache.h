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

#pragma once
#include "HashCacheDelta.h"
#include "HashCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace config { class BlockchainConfigurationHolder; } }

namespace catapult { namespace cache {

	using HashBasicCache = BasicCache<HashCacheDescriptor, HashCacheTypes::BaseSets, HashCacheTypes::Options>;

	/// Cache composed of timestamped hashes of (transaction) elements.
	/// \note The cache can be pruned according to the retention time.
	class BasicHashCache : public HashBasicCache {
	public:
		/// Creates a cache around \a config with the specified \a pConfigHolder.
		explicit BasicHashCache(const CacheConfiguration& config, const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
				// hash cache should always be excluded from state hash calculation
				: HashBasicCache(DisablePatriciaTreeStorage(config), HashCacheTypes::Options{ pConfigHolder })
		{}

	private:
		static CacheConfiguration DisablePatriciaTreeStorage(const CacheConfiguration& config) {
			auto configCopy = config;
			configCopy.ShouldStorePatriciaTrees = false;
			return configCopy;
		}
	};

	/// Synchronized cache composed of timestamped hashes of (transaction) elements.
	class HashCache : public SynchronizedCache<BasicHashCache> {
	public:
		DEFINE_CACHE_CONSTANTS(Hash)

	public:
		/// Creates a cache around \a config with the specified \a pConfigHolder.
		explicit HashCache(const CacheConfiguration& config, const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
				: SynchronizedCache<BasicHashCache>(BasicHashCache(config, pConfigHolder))
		{}
	};
}}
