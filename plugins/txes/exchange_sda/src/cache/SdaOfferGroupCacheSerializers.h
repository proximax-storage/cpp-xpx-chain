/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SdaOfferGroupCacheTypes.h"
#include "src/state/SdaOfferGroupEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"
#include "catapult/cache/IdentifierGroupSerializer.h"

namespace catapult { namespace cache {

	/// Primary serializer for SDA-SDA offer group cache.
	struct SdaOfferGroupEntryPrimarySerializer : public CacheSerializerAdapter<state::SdaOfferGroupEntrySerializer, SdaOfferGroupCacheDescriptor> {};
}}
