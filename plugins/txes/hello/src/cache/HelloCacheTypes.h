/**
*** FOR TRAINING PURPOSES ONLY
**/

#pragma once
#include "src/state/HelloEntry.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/utils/Hashers.h"

namespace catapult {
    namespace cache {
        class BasicHelloCacheDelta;
        class BasicHelloCacheView;
        struct HelloBaseSetDeltaPointers;
        struct HelloBaseSets;
        class HelloCache;
        class HelloCacheDelta;
        class HelloCacheView;
        struct HelloEntryPrimarySerializer;
        class HelloPatriciaTree;

        template<typename TCache, typename TCacheDelta, typename TKey, typename TGetResult>
        class ReadOnlyArtifactCache;
    }
}

namespace catapult { namespace cache {

        /// Describes a catapult upgrade cache.
        struct HelloCacheDescriptor {
        public:
            static constexpr auto Name = "HelloCache";

        public:
            // key value types
            using KeyType = Key;                     //? pending question to Oleg
            using ValueType = state::HelloEntry;

            // cache types
            using CacheType = HelloCache;
            using CacheDeltaType = HelloCacheDelta;
            using CacheViewType = HelloCacheView;

            using Serializer = HelloEntryPrimarySerializer;
            using PatriciaTree = HelloPatriciaTree;

        public:
            /// Gets the key corresponding to \a entry.
            static const auto& GetKeyFromValue(const ValueType& entry) {
                return entry.key();
            }
        };

        /// Catapult upgrade cache types.
        struct HelloCacheTypes {

            // important, when using Key as type, use ArrayHasher and not BaseValueHasher
            // data type Key is defined as a utils::ByteArray
            // BaseValueHasher is for utils::BaseValue type
            using PrimaryTypes = MutableUnorderedMapAdapter<HelloCacheDescriptor, utils::ArrayHasher<Key>>;

            using CacheReadOnlyType = ReadOnlyArtifactCache<BasicHelloCacheView, BasicHelloCacheDelta, const Key&, state::HelloEntry>;

            using BaseSetDeltaPointers = HelloBaseSetDeltaPointers;
            using BaseSets = HelloBaseSets;
        };
    }}
