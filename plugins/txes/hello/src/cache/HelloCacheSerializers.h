/**
*** FOR TRAINING PURPOSES ONLY
**/

#pragma once
#include "HelloCacheTypes.h"
#include "src/state/HelloEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"

namespace catapult { namespace cache {

        /// Primary serializer for catapult upgrade cache.
        struct HelloEntryPrimarySerializer : public CacheSerializerAdapter<state::HelloEntrySerializer, HelloCacheDescriptor>
        {};
    }}
