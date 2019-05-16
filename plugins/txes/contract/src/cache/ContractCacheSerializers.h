/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ContractCacheTypes.h"
#include "src/state/ContractEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"

namespace catapult { namespace cache {

	/// Primary serializer for contract cache.
	struct ContractEntryPrimarySerializer : public CacheSerializerAdapter<state::ContractEntrySerializer, ContractCacheDescriptor>
	{};
}}
