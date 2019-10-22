/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OfferCacheTypes.h"
#include "src/state/OfferEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"
#include "catapult/cache/IdentifierGroupSerializer.h"

namespace catapult { namespace cache {

	/// Primary serializer for offer cache.
	struct OfferEntryPrimarySerializer : public CacheSerializerAdapter<state::OfferEntrySerializer, OfferCacheDescriptor> {};

	/// Serializer for offer cache height grouped elements.
	struct OfferHeightGroupingSerializer : public IdentifierGroupSerializer<OfferCacheTypes::HeightGroupingTypesDescriptor> {};
}}
