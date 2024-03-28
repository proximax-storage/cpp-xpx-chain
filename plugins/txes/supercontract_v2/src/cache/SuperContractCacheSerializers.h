/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SuperContractCacheTypes.h"
#include "src/state/SuperContractEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"
#include "catapult/cache/IdentifierGroupSerializer.h"

namespace catapult { namespace cache {

	/// Primary serializer for super SuperContract cache.
	struct SuperContractEntryPrimarySerializer : public CacheSerializerAdapter<state::SuperContractEntrySerializer, SuperContractCacheDescriptor>
	{};
}}
