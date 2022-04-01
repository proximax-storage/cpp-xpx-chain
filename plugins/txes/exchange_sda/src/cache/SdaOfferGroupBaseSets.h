/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SdaOfferGroupCacheSerializers.h"
#include "SdaOfferGroupCacheTypes.h"
#include "catapult/cache/CachePatriciaTree.h"
#include "catapult/cache/PatriciaTreeEncoderAdapters.h"
#include "catapult/cache/SingleSetCacheTypesAdapter.h"
#include "catapult/tree/BasePatriciaTree.h"

namespace catapult { namespace cache {

    using BasicSdaOfferGroupPatriciaTree = tree::BasePatriciaTree<
        SerializerHashedKeyEncoder<SdaOfferGroupCacheDescriptor::Serializer>,
        PatriciaTreeRdbDataSource,
        utils::ArrayHasher<Hash256>>;

    class SdaOfferGroupPatriciaTree : public BasicSdaOfferGroupPatriciaTree {
    public:
        using BasicSdaOfferGroupPatriciaTree::BasicSdaOfferGroupPatriciaTree;
        using Serializer = SdaOfferGroupCacheDescriptor::Serializer;
    };

    using SdaOfferGroupSingleSetCacheTypesAdapter =
        SingleSetAndPatriciaTreeCacheTypesAdapter<SdaOfferGroupCacheTypes::PrimaryTypes, SdaOfferGroupPatriciaTree>;

    struct SdaOfferGroupBaseSetDeltaPointers {
        SdaOfferGroupCacheTypes::PrimaryTypes::BaseSetDeltaPointerType pPrimary;
        std::shared_ptr<SdaOfferGroupPatriciaTree::DeltaType> pPatriciaTree;
    };

    struct SdaOfferGroupBaseSets : public CacheDatabaseMixin {
    public:
        /// Indicates the set is not ordered.
        using IsOrderedSet = std::false_type;

    public:
        explicit SdaOfferGroupBaseSets(const CacheConfiguration& config)
                : CacheDatabaseMixin(config, { "default" })
                , Primary(GetContainerMode(config), database(), 0)
                , PatriciaTree(hasPatriciaTreeSupport(), database(), 1)
        {}

    public:
        SdaOfferGroupCacheTypes::PrimaryTypes::BaseSetType Primary;
        CachePatriciaTree<SdaOfferGroupPatriciaTree> PatriciaTree;

    public:
        SdaOfferGroupBaseSetDeltaPointers rebase() {
            return { Primary.rebase(), PatriciaTree.rebase() };
        }

        SdaOfferGroupBaseSetDeltaPointers rebaseDetached() const {
            return { Primary.rebaseDetached(), PatriciaTree.rebaseDetached() };
        }

        void commit() {
            Primary.commit();
            PatriciaTree.commit();
            flush();
        }
    };
}}
