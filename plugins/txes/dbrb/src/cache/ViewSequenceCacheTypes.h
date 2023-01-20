/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/MessageHashEntry.h"
#include "src/state/ViewSequenceEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/utils/Hashers.h"
#include "catapult/utils/IdentifierGroup.h"

namespace catapult {
	namespace cache {
		class BasicViewSequenceCacheDelta;
		class BasicViewSequenceCacheView;
		struct ViewSequenceBaseSetDeltaPointers;
		struct ViewSequenceBaseSets;
		class ViewSequenceCache;
		class ViewSequenceCacheDelta;
		class ViewSequenceCacheView;
		struct ViewSequenceEntryPrimarySerializer;
		class ViewSequencePatriciaTree;
		struct MessageHashEntrySerializer;
		struct ReadOnlyViewSequenceCache;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a View sequence cache.
	struct ViewSequenceCacheDescriptor {
	public:
		static constexpr auto Name = "ViewSequenceCache";

	public:
		// key value types
		using KeyType = Hash256;
		using ValueType = state::ViewSequenceEntry;

		// cache types
		using CacheType = ViewSequenceCache;
		using CacheDeltaType = ViewSequenceCacheDelta;
		using CacheViewType = ViewSequenceCacheView;

		using Serializer = ViewSequenceEntryPrimarySerializer;
		using PatriciaTree = ViewSequencePatriciaTree;

	public:
		/// Gets the hash corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.hash();
		}
	};

	/// View sequence cache types.
	struct ViewSequenceCacheTypes {
		struct MessageHashTypesDescriptor {
		public:
			using KeyType = uint8_t;
			using ValueType = state::MessageHashEntry;
			using Serializer = MessageHashEntrySerializer;

		public:
			static const auto& GetKeyFromValue(const ValueType& entry) {
				return entry.key();
			}
		};

		using PrimaryTypes = MutableUnorderedMapAdapter<ViewSequenceCacheDescriptor, utils::ArrayHasher<Hash256>>;
		using MessageHashTypes = MutableUnorderedMapAdapter<MessageHashTypesDescriptor>;

		using CacheReadOnlyType = ReadOnlyViewSequenceCache;

		using BaseSetDeltaPointers = ViewSequenceBaseSetDeltaPointers;
		using BaseSets = ViewSequenceBaseSets;
	};
}}
