/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/LockFundRecordGroup.h"
#include "src/state/LockFundRecord.h"
#include "catapult/cache/ReadOnlyComplexArtifactCache.h"
#include "catapult/cache/CacheDatabaseMixin.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/IdentifierGroupSerializer.h"
#include "catapult/utils/Hashers.h"
#include "catapult/utils/IdentifierGroup.h"

namespace catapult {
	namespace cache {
		class BasicLockFundCacheView;
		class BasicLockFundCacheDelta;
		struct LockFundBaseSetDeltaPointers;
		struct LockFundBaseSets;
		class LockFundCache;
		class LockFundCacheDelta;
		struct LockFundCacheTypes;
		class LockFundCacheView;
		class LockFundPatriciaTree;
		struct LockFundPrimarySerializer;
		struct KeyedLockFundSerializer;
	}
}

namespace catapult { namespace cache {

	/// Describes a lock fund info cache.
	struct LockFundCacheDescriptor {
	public:
		static constexpr auto Name = "LockFundCache";

	public:
		// key value types
		using KeyType = Height;
		using ValueType = state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>;

		// cache types
		using CacheType = LockFundCache;
		using CacheDeltaType = LockFundCacheDelta;
		using CacheViewType = LockFundCacheView;

		using Serializer = LockFundPrimarySerializer;
		using PatriciaTree = LockFundPatriciaTree;

	public:
		/// Gets the key corresponding to \a lockInfo.
		static const auto& GetKeyFromValue(const ValueType& LockFund) {
			return LockFund.Identifier;
		}
	};


	struct LockFundCacheTypes {


	private:
		using IdentifierType = LockFundCacheDescriptor::KeyType;

		// region secondary descriptors

	public:

		struct KeyedLockFundTypesDescriptor {
		public:
			using KeyType = Key;
			using CacheType = LockFundCacheDescriptor::CacheType;
			using CacheDeltaType = LockFundCacheDescriptor::CacheDeltaType;
			using ValueType = state::LockFundRecordGroup<state::LockFundKeyIndexDescriptor>;
			using Serializer = KeyedLockFundSerializer;
			static constexpr auto Name = "LockFundCacheKeyed";

		public:
			static auto GetKeyFromValue(const ValueType& LockFund) {
				return LockFund.Identifier;
			}
		};

		// endregion
	public:
		using CacheReadOnlyType = ReadOnlyComplexArtifactCache<
				BasicLockFundCacheView,
				BasicLockFundCacheDelta,
				LockFundCacheDescriptor,
				KeyedLockFundTypesDescriptor>;
	public:
		using PrimaryTypes = MutableUnorderedMapAdapter<LockFundCacheDescriptor, utils::BaseValueHasher<LockFundCacheDescriptor::KeyType>>;
		using KeyedLockFundTypes = MutableUnorderedMapAdapter<KeyedLockFundTypesDescriptor, utils::ArrayHasher<KeyedLockFundTypesDescriptor::KeyType>>;


		using BaseSetDeltaPointers = LockFundBaseSetDeltaPointers;
		using BaseSets = LockFundBaseSets;
	};
}}
