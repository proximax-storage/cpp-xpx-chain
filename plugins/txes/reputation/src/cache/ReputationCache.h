/**
*** Copyright (c) 2018-present,
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
#include "ReputationCacheDelta.h"
#include "ReputationCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of reputation information.
	using BasicReputationCache = BasicCache<ReputationCacheDescriptor, ReputationCacheTypes::BaseSets>;

	/// Synchronized cache composed of reputation information.
	class ReputationCache : public SynchronizedCache<BasicReputationCache> {
	public:
		DEFINE_CACHE_CONSTANTS(Reputation)

	public:
		/// Creates a cache around \a config.
		explicit ReputationCache(const CacheConfiguration& config) : SynchronizedCache<BasicReputationCache>(BasicReputationCache(config))
		{}
	};
}}
