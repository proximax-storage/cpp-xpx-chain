/**
*** FOR TRAINING PURPOSES ONLY
**/

#pragma once
#include "src/cache/HelloCache.h"
#include "src/cache/HelloCacheStorage.h"
#include "tests/test/cache/CacheTestUtils.h"

namespace catapult { namespace cache { class CatapultCacheDelta; } }

namespace catapult { namespace test {

        /// Cache factory for creating a catapult cache composed of only the config cache.
        struct HelloCacheFactory {
            /// Creates an empty catapult cache.
            static cache::CatapultCache Create() {
                auto cacheId = cache::HelloCache::Id;
                std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cacheId + 1);
                subCaches[cacheId] = test::MakeSubCachePlugin<cache::HelloCache, cache::HelloCacheStorage>();
                return cache::CatapultCache(std::move(subCaches));
            }
        };

        inline std::vector<Key> GenerateKeys(size_t count) {
            return test::GenerateRandomDataVector<Key>(count);
        }
    }}


