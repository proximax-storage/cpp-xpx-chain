/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DealCacheTypes.h"
#include "src/state/DealEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"

namespace catapult { namespace cache {

	/// Primary serializer for deal cache.
	struct DealEntryPrimarySerializer : public CacheSerializerAdapter<state::DealEntrySerializer, DealCacheDescriptor>
	{};
}}
