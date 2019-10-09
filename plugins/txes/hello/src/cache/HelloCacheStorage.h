/**
*** FOR TRAINING PURPOSES ONLY
**/

#pragma once
#include "HelloCacheTypes.h"
#include "src/state/HelloEntrySerializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

        /// Policy for saving and loading catapult upgrade cache data.
        struct HelloCacheStorage
                : public CacheStorageForBasicInsertRemoveCache<HelloCacheDescriptor>
                        , public state::HelloEntrySerializer {

            /// Loads \a entry into \a cacheDelta.
            static void LoadInto(const ValueType& entry, DestinationType& cacheDelta);
        };
    }}
