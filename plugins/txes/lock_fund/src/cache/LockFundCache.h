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
#include "catapult/cache/BasicCache.h"
#include "LockFundCacheView.h"
#include "LockFundCacheDelta.h"
#include "src/state/LockFundRecordGroup.h"
#include "LockFundCacheTypes.h"

namespace catapult { namespace cache {

	/// Cache composed of hash lock info information.
	using BasicLockFundCache = BasicCache<LockFundCacheDescriptor, LockFundCacheTypes::BaseSets, std::shared_ptr<config::BlockchainConfigurationHolder>>;

	/// Synchronized cache composed of hash lock info information.
	class LockFundCache : public SynchronizedCache<BasicLockFundCache> {
	public:
		DEFINE_CACHE_CONSTANTS(LockFund)

		using CacheValueTypes = std::tuple<typename LockFundCache::CacheValueType, state::LockFundRecordGroup<state::LockFundKeyIndexDescriptor>>;
	public:
		/// Creates a cache around \a config.
		explicit LockFundCache(const CacheConfiguration& config, std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: SynchronizedCache<BasicLockFundCache>(BasicLockFundCache(config, std::move(pConfigHolder)))
		{}


	};
}}
