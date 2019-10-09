/**
*** FOR TRAINING PURPOSES ONLY
**/


#pragma once
#include "HelloBaseSets.h"
#include "HelloCacheSerializers.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"

namespace catapult { namespace cache {

        /// Mixins used by the catapult upgrade cache view.
        using HelloCacheViewMixins = PatriciaTreeCacheMixins<HelloCacheTypes::PrimaryTypes::BaseSetType, HelloCacheDescriptor>;

        /// Basic view on top of the catapult upgrade cache.
        class BasicHelloCacheView
                : public utils::MoveOnly
                        , public HelloCacheViewMixins::Size
                        , public HelloCacheViewMixins::Contains
                        , public HelloCacheViewMixins::Iteration
                        , public HelloCacheViewMixins::ConstAccessor
                        , public HelloCacheViewMixins::PatriciaTreeView
                        , public HelloCacheViewMixins::Enable
                        , public HelloCacheViewMixins::Height {
        public:
            using ReadOnlyView = HelloCacheTypes::CacheReadOnlyType;

        public:
            /// Creates a view around \a HelloSets.
            explicit BasicHelloCacheView(const HelloCacheTypes::BaseSets& helloSets)
                    : HelloCacheViewMixins::Size(helloSets.Primary)
                    , HelloCacheViewMixins::Contains(helloSets.Primary)
                    , HelloCacheViewMixins::Iteration(helloSets.Primary)
                    , HelloCacheViewMixins::ConstAccessor(helloSets.Primary)
                    , HelloCacheViewMixins::PatriciaTreeView(helloSets.PatriciaTree.get())
            {}
        };

        /// View on top of the catapult upgrade cache.
        class HelloCacheView : public ReadOnlyViewSupplier<BasicHelloCacheView> {
        public:
            /// Creates a view around \a HelloSets.
            explicit HelloCacheView(const HelloCacheTypes::BaseSets& helloSets)
                    : ReadOnlyViewSupplier(helloSets)
            {}
        };
    }}
