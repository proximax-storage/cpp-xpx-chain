/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DbrbViewCacheTypes.h"
#include "src/state/DbrbProcessEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"
#include "catapult/cache/IdentifierGroupSerializer.h"

namespace catapult { namespace cache {

	/// Primary serializer for DBRB view cache.
	struct DbrbProcessEntryPrimarySerializer : public CacheSerializerAdapter<state::DbrbProcessEntrySerializer, DbrbViewCacheDescriptor>
	{};
}}
