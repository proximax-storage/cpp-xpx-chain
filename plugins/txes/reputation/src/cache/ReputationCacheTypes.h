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
#include "src/state/ReputationEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicReputationCacheDelta;
		class BasicReputationCacheView;
		struct ReputationBaseSetDeltaPointers;
		struct ReputationBaseSets;
		class ReputationCache;
		class ReputationCacheDelta;
		class ReputationCacheView;
		struct ReputationEntryPrimarySerializer;
		class ReputationPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a reputation cache.
	struct ReputationCacheDescriptor {
	public:
		static constexpr auto Name = "ReputationCache";

	public:
		// key value types
		using KeyType = Key;
		using ValueType = state::ReputationEntry;

		// cache types
		using CacheType = ReputationCache;
		using CacheDeltaType = ReputationCacheDelta;
		using CacheViewType = ReputationCacheView;

		using Serializer = ReputationEntryPrimarySerializer;
		using PatriciaTree = ReputationPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.key();
		}
	};

	/// Reputation cache types.
	struct ReputationCacheTypes {
		using PrimaryTypes = MutableUnorderedMapAdapter<ReputationCacheDescriptor, utils::ArrayHasher<Key>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicReputationCacheView, BasicReputationCacheDelta, const Key&, state::ReputationEntry>;

		using BaseSetDeltaPointers = ReputationBaseSetDeltaPointers;
		using BaseSets = ReputationBaseSets;
	};
}}
