/**
*** FOR TRAINING PURPOSES ONLY
**/

#pragma once
#include "HelloCacheSerializers.h"
#include "HelloCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

        using BasicHelloPatriciaTree = tree::BasePatriciaTree<
                SerializerHashedKeyEncoder<HelloCacheDescriptor::Serializer>,
                PatriciaTreeRdbDataSource,
                utils::ArrayHasher<Key>>;

        class HelloPatriciaTree : public BasicHelloPatriciaTree {
        public:
            using BasicHelloPatriciaTree::BasicHelloPatriciaTree;
            using Serializer = HelloCacheDescriptor::Serializer;
        };

        using HelloSingleSetCacheTypesAdapter =
        SingleSetAndPatriciaTreeCacheTypesAdapter<HelloCacheTypes::PrimaryTypes, HelloPatriciaTree>;

        struct HelloBaseSetDeltaPointers : public HelloSingleSetCacheTypesAdapter::BaseSetDeltaPointers {
        };

        struct HelloBaseSets : public HelloSingleSetCacheTypesAdapter::BaseSets<HelloBaseSetDeltaPointers> {
            using HelloSingleSetCacheTypesAdapter::BaseSets<HelloBaseSetDeltaPointers>::BaseSets;
        };
    }}

