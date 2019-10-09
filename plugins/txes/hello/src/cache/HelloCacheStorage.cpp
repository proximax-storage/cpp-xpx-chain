/**
*** FOR TRAINING PURPOSES ONLY
**/

#include "HelloCacheStorage.h"
#include "HelloCacheDelta.h"

namespace catapult { namespace cache {

        void HelloCacheStorage::LoadInto(const ValueType& entry, DestinationType& cacheDelta) {
            cacheDelta.insert(entry);
        }
    }}
