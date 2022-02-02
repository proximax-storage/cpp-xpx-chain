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


	using LockFundKeyIndexDescriptor = state::LockFundRecordGroupIndexDescriptor<Key, Height, utils::BaseValueHasher<Height>>;
	using LockFundHeightIndexDescriptor = state::LockFundRecordGroupIndexDescriptor<Height, Key, utils::ArrayHasher<Key>>;
	/// Describes a lock fund info cache.
	struct LockFundCacheDescriptor {
	public:
		static constexpr auto Name = "LockFundCache";

	public:
		// key value types
		using KeyType = Height;
		using ValueType = state::LockFundRecordGroup<LockFundHeightIndexDescriptor>;

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
			using ValueType = state::LockFundRecordGroup<LockFundKeyIndexDescriptor>;
			using Serializer = KeyedLockFundSerializer;

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
