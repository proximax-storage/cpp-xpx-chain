/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "QueueCacheTypes.h"
#include "src/state/QueueEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"

namespace catapult { namespace cache {

	/// Primary serializer for drive cache.
	struct QueueEntryPrimarySerializer : public CacheSerializerAdapter<state::QueueEntrySerializer, QueueCacheDescriptor>
	{};
}}
