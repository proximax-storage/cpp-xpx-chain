/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "NetworkConfigCacheTypes.h"
#include "plugins/txes/config/src/state/NetworkConfigEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"

namespace catapult { namespace cache {

	/// Primary serializer for network config cache.
	struct NetworkConfigEntryPrimarySerializer : public CacheSerializerAdapter<state::NetworkConfigEntrySerializer, NetworkConfigCacheDescriptor>
	{};
}}
