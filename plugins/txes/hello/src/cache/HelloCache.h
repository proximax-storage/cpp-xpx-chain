/**
*** FOR TRAINING PURPOSES ONLY
**/

#pragma once
#include "HelloCacheDelta.h"
#include "HelloCacheView.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

        /// Cache composed of catapult upgrade information.
        using BasicHelloCache = BasicCache<HelloCacheDescriptor, HelloCacheTypes::BaseSets>;

        /// Synchronized cache composed of catapult upgrade information.
        class HelloCache : public SynchronizedCache<BasicHelloCache> {
        public:

            //expands to Hello "Cache" string
            // and CacheId::Hello --> should be defined in \src\catapult\cache\CacheConstants.h
            DEFINE_CACHE_CONSTANTS(Hello)

        public:
            /// Creates a cache around \a config.
            explicit HelloCache(const CacheConfiguration& config) : SynchronizedCache<BasicHelloCache>(BasicHelloCache(config))
            {}
        };
    }}
