/**
*** FOR TRAINING PURPOSES ONLY
**/

#pragma once
#include "HelloBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

        /// Mixins used by the catapult upgrade cache delta.
        using HelloCacheDeltaMixins = PatriciaTreeCacheMixins<HelloCacheTypes::PrimaryTypes::BaseSetDeltaType, HelloCacheDescriptor>;

        /// Basic delta on top of the catapult upgrade cache.
        class BasicHelloCacheDelta
                : public utils::MoveOnly
                        , public HelloCacheDeltaMixins::Size
                        , public HelloCacheDeltaMixins::Contains
                        , public HelloCacheDeltaMixins::ConstAccessor
                        , public HelloCacheDeltaMixins::MutableAccessor
                        , public HelloCacheDeltaMixins::PatriciaTreeDelta
                        , public HelloCacheDeltaMixins::BasicInsertRemove
                        , public HelloCacheDeltaMixins::DeltaElements
                        , public HelloCacheDeltaMixins::Enable
                        , public HelloCacheDeltaMixins::Height {
        public:
            using ReadOnlyView = HelloCacheTypes::CacheReadOnlyType;

        public:
            /// Creates a delta around \a HelloSets.
            explicit BasicHelloCacheDelta(const HelloCacheTypes::BaseSetDeltaPointers& helloSets)
                    : HelloCacheDeltaMixins::Size(*helloSets.pPrimary)
                    , HelloCacheDeltaMixins::Contains(*helloSets.pPrimary)
                    , HelloCacheDeltaMixins::ConstAccessor(*helloSets.pPrimary)
                    , HelloCacheDeltaMixins::MutableAccessor(*helloSets.pPrimary)
                    , HelloCacheDeltaMixins::PatriciaTreeDelta(*helloSets.pPrimary, helloSets.pPatriciaTree)
                    , HelloCacheDeltaMixins::BasicInsertRemove(*helloSets.pPrimary)
                    , HelloCacheDeltaMixins::DeltaElements(*helloSets.pPrimary)
                    , m_pHelloEntries(helloSets.pPrimary)
            {}

        public:
            using HelloCacheDeltaMixins::ConstAccessor::find;
            using HelloCacheDeltaMixins::MutableAccessor::find;

        private:
            HelloCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pHelloEntries;
        };

        /// Delta on top of the catapult upgrade cache.
        class HelloCacheDelta : public ReadOnlyViewSupplier<BasicHelloCacheDelta> {
        public:
            /// Creates a delta around \a HelloSets.
            explicit HelloCacheDelta(const HelloCacheTypes::BaseSetDeltaPointers& helloSets)
                    : ReadOnlyViewSupplier(helloSets)
            {}
        };
    }}
