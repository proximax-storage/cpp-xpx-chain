/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "LiquidityProviderCacheTypes.h"
#include "src/state/LPEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"

namespace catapult { namespace cache {

	/// Primary serializer for LiquidityProvider cache.
	struct LiquidityProviderEntryPrimarySerializer : public CacheSerializerAdapter<state::LiquidityProviderEntrySerializer, LiquidityProviderCacheDescriptor>
	{};
}}
