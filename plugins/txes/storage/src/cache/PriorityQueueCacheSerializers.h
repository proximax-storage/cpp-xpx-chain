/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "PriorityQueueCacheTypes.h"
#include "src/state/PriorityQueueEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"
#include "catapult/cache/IdentifierGroupSerializer.h"

namespace catapult { namespace cache {

	/// Primary serializer for priority queue cache.
	struct PriorityQueueEntryPrimarySerializer : public CacheSerializerAdapter<state::PriorityQueueEntrySerializer, PriorityQueueCacheDescriptor>
	{};
}}
