/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OperationCacheTypes.h"
#include "src/state/OperationEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"

namespace catapult { namespace cache {

	/// Primary serializer for operation cache.
	struct OperationPrimarySerializer : public CacheSerializerAdapter<state::OperationEntrySerializer, OperationCacheDescriptor>
	{};
}}
