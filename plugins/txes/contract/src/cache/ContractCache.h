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
#include "ContractCacheDelta.h"
#include "ContractCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of contract information.
	using BasicContractCache = BasicCache<ContractCacheDescriptor, ContractCacheTypes::BaseSets>;

	/// Synchronized cache composed of contract information.
	class ContractCache : public SynchronizedCache<BasicContractCache> {
	public:
		DEFINE_CACHE_CONSTANTS(Contract)

	public:
		/// Creates a cache around \a config.
		explicit ContractCache(const CacheConfiguration& config) : SynchronizedCache<BasicContractCache>(BasicContractCache(config))
		{}
	};
}}
