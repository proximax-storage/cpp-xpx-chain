/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/OperationEntry.h"
#include "plugins/txes/lock_shared/src/cache/LockInfoCacheTypes.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"

namespace catapult {
	namespace cache {
		class BasicOperationCacheDelta;
		class BasicOperationCacheView;
		struct OperationBaseSetDeltaPointers;
		struct OperationBaseSets;
		class OperationCache;
		class OperationCacheDelta;
		class OperationCacheView;
		class OperationPatriciaTree;
		struct OperationPrimarySerializer;
	}
}

namespace catapult { namespace cache {

	/// Describes a operation cache.
	struct OperationCacheDescriptor {
	public:
		static constexpr auto Name = "OperationCache";

	public:
		// key value types
		using KeyType = Hash256;
		using ValueType = state::OperationEntry;

		// cache types
		using CacheType = OperationCache;
		using CacheDeltaType = OperationCacheDelta;
		using CacheViewType = OperationCacheView;

		using Serializer = OperationPrimarySerializer;
		using PatriciaTree = OperationPatriciaTree;

	public:
		/// Gets the key corresponding to \a lockInfo.
		static const auto& GetKeyFromValue(const ValueType& lockInfo) {
			return lockInfo.OperationToken;
		}
	};

	/// Operation info cache types.
	struct OperationCacheTypes : public LockInfoCacheTypes<OperationCacheDescriptor> {
		using CacheReadOnlyType = ReadOnlyArtifactCache<
			BasicOperationCacheView,
			BasicOperationCacheDelta,
			Hash256,
			state::OperationEntry>;

		using BaseSetDeltaPointers = OperationBaseSetDeltaPointers;
		using BaseSets = OperationBaseSets;
	};
}}
