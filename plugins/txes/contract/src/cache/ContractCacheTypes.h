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
#include "src/state/ContractEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicContractCacheDelta;
		class BasicContractCacheView;
		struct ContractBaseSetDeltaPointers;
		struct ContractBaseSets;
		class ContractCache;
		class ContractCacheDelta;
		class ContractCacheView;
		struct ContractEntryPrimarySerializer;
		class ContractPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a contract cache.
	struct ContractCacheDescriptor {
	public:
		static constexpr auto Name = "ContractCache";

	public:
		// key value types
		using KeyType = Key;
		using ValueType = state::ContractEntry;

		// cache types
		using CacheType = ContractCache;
		using CacheDeltaType = ContractCacheDelta;
		using CacheViewType = ContractCacheView;

		using Serializer = ContractEntryPrimarySerializer;
		using PatriciaTree = ContractPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.key();
		}
	};

	/// Contract cache types.
	struct ContractCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<ContractCacheDescriptor, utils::ArrayHasher<Key>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicContractCacheView, BasicContractCacheDelta, const Key&, state::ContractEntry>;

		using BaseSetDeltaPointers = ContractBaseSetDeltaPointers;
		using BaseSets = ContractBaseSets;
	};
}}
