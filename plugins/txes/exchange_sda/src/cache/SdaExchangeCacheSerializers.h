/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SdaExchangeCacheTypes.h"
#include "src/state/SdaExchangeEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"
#include "catapult/cache/IdentifierGroupSerializer.h"

namespace catapult { namespace cache {

	/// Primary serializer for SDA-SDA exchange cache.
	struct SdaExchangeEntryPrimarySerializer : public CacheSerializerAdapter<state::SdaExchangeEntrySerializer, SdaExchangeCacheDescriptor> {};

	/// Primary serializer for SDA-SDA exchange cache for patricia tree hashes.
	/// \note This serializer excludes expired offers.
	struct SdaExchangeEntryPatriciaTreeSerializer : public CacheSerializerAdapter<state::SdaExchangeEntryNonHistoricalSerializer, SdaExchangeCacheDescriptor> {};

	/// Serializer for SDA-SDA exchange cache height grouped elements.
	struct SdaExchangeHeightGroupingSerializer : public IdentifierGroupSerializer<SdaExchangeCacheTypes::HeightGroupingTypesDescriptor> {};
}}
