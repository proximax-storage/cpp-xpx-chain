/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ReputationCacheTypes.h"
#include "src/state/ReputationEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"

namespace catapult { namespace cache {

	/// Primary serializer for reputation cache.
	struct ReputationEntryPrimarySerializer : public CacheSerializerAdapter<state::ReputationEntrySerializer, ReputationCacheDescriptor>
	{};
}}
