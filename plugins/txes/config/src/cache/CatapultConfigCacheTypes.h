/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "plugins/txes/config/src/state/CatapultConfigEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/IdentifierGroupSerializer.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/IdentifierGroup.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
	namespace cache {
		class BasicCatapultConfigCacheDelta;
		class BasicCatapultConfigCacheView;
		struct CatapultConfigBaseSetDeltaPointers;
		struct CatapultConfigBaseSets;
		class CatapultConfigCache;
		class CatapultConfigCacheDelta;
		class CatapultConfigCacheView;
		struct CatapultConfigEntryPrimarySerializer;
		class CatapultConfigPatriciaTree;

		template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
		class ReadOnlyArtifactCache;
	}
}

namespace catapult { namespace cache {

	/// Describes a catapult config cache.
	struct CatapultConfigCacheDescriptor {
	public:
		static constexpr auto Name = "CatapultConfigCache";

	public:
		// key value types
		using KeyType = Height;
		using ValueType = state::CatapultConfigEntry;

		// cache types
		using CacheType = CatapultConfigCache;
		using CacheDeltaType = CatapultConfigCacheDelta;
		using CacheViewType = CatapultConfigCacheView;

		using Serializer = CatapultConfigEntryPrimarySerializer;
		using PatriciaTree = CatapultConfigPatriciaTree;

	public:
		/// Gets the key corresponding to \a entry.
		static const auto& GetKeyFromValue(const ValueType& entry) {
			return entry.height();
		}
	};

	template<typename TDescriptor>
	class HeightSerializer {
	private:
		using ValueType = typename TDescriptor::ValueType;

	public:
		/// Serializes \a value to string.
		static std::string SerializeValue(const ValueType& value) {
			io::StringOutputStream output(sizeof(VersionType) + sizeof(ValueType));

			// write version
			io::Write32(output, 1);

			io::Write(output, value);

			return output.str();
		}

		/// Deserializes value from \a buffer.
		static ValueType DeserializeValue(const RawBuffer& buffer) {
			io::BufferInputStreamAdapter<RawBuffer> input(buffer);

			// read version
			VersionType version = io::Read32(input);
			if (version > 1)
				CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of height", version);

			ValueType value = io::Read<Height>(input);

			return value;
		}
	};

	/// Catapult config cache types.
	struct CatapultConfigCacheTypes {
		using CacheReadOnlyType = ReadOnlyArtifactCache<BasicCatapultConfigCacheView, BasicCatapultConfigCacheDelta, const Height&, state::CatapultConfigEntry>;

		// region secondary descriptors

		struct HeightTypesDescriptor {
		public:
			using ValueType = Height;
			using Serializer = HeightSerializer<HeightTypesDescriptor>;

		public:
			static auto GetKeyFromValue(const ValueType& height) {
				return height;
			}
		};

		// endregion

		using PrimaryTypes = MutableUnorderedMapAdapter<CatapultConfigCacheDescriptor, utils::BaseValueHasher<Height>>;
		using HeightTypes = MutableOrderedMemorySetAdapter<HeightTypesDescriptor>;

		using BaseSetDeltaPointers = CatapultConfigBaseSetDeltaPointers;
		using BaseSets = CatapultConfigBaseSets;
	};
}}
