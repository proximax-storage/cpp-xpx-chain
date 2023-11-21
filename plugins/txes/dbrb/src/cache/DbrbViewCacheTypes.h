/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/DbrbProcessEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/IdentifierSerializer.h"
#include "catapult/utils/Hashers.h"
#include "catapult/utils/IdentifierGroup.h"

namespace catapult {
	namespace cache {
		class BasicDbrbViewCacheDelta;
		class BasicDbrbViewCacheView;
		struct DbrbViewBaseSetDeltaPointers;
		struct DbrbViewBaseSets;
		class DbrbViewCache;
		class DbrbViewCacheDelta;
		class DbrbViewCacheView;
		struct DbrbProcessEntryPrimarySerializer;
		class DbrbViewPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a DBRB view cache.
	struct DbrbViewCacheDescriptor {
	public:
		static constexpr auto Name = "DbrbViewCache";

	public:
		// key value types
		using KeyType = dbrb::ProcessId;
		using ValueType = state::DbrbProcessEntry;

		// cache types
		using CacheType = DbrbViewCache;
		using CacheDeltaType = DbrbViewCacheDelta;
		using CacheViewType = DbrbViewCacheView;

		using Serializer = DbrbProcessEntryPrimarySerializer;
		using PatriciaTree = DbrbViewPatriciaTree;

	public:
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.processId();
		}
	};

	/// DBRB view cache types.
	struct DbrbViewCacheTypes {
		struct ProcessIdTypesDescriptor {
		public:
			using KeyType = dbrb::ProcessId;
			using ValueType = dbrb::ProcessId;
			using Serializer = IdentifierSerializer<ProcessIdTypesDescriptor>;

		public:
			static auto GetKeyFromValue(const ValueType& processId) {
				return processId;
			}

			static uint64_t KeyToBoundary(const ValueType&) {
				return 0u;
			}
		};

		using PrimaryTypes = MutableUnorderedMapAdapter<DbrbViewCacheDescriptor, utils::ArrayHasher<dbrb::ProcessId>>;
		using ProcessIdTypes = MutableUnorderedMemorySetAdapter<ProcessIdTypesDescriptor, utils::ArrayHasher<dbrb::ProcessId>>;

		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicDbrbViewCacheView, BasicDbrbViewCacheDelta, const dbrb::ProcessId&, state::DbrbProcessEntry>;

		using BaseSetDeltaPointers = DbrbViewBaseSetDeltaPointers;
		using BaseSets = DbrbViewBaseSets;
	};
}}
