/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ExchangeCacheTypes.h"
#include "src/state/ExchangeEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"
#include "catapult/cache/IdentifierGroupSerializer.h"

namespace catapult { namespace cache {

	/// Primary serializer for exchange cache.
	struct ExchangeEntryPrimarySerializer : public CacheSerializerAdapter<state::ExchangeEntrySerializer, ExchangeCacheDescriptor> {};

	/// Primary serializer for exchange cache for patricia tree hashes.
	/// \note This serializer excludes expired offers.
	struct ExchangeEntryPatriciaTreeSerializer : public CacheSerializerAdapter<state::ExchangeEntryNonHistoricalSerializer, ExchangeCacheDescriptor> {};

	/// Serializer for exchange cache height grouped elements.
	struct ExchangeHeightGroupingSerializer : public IdentifierGroupSerializer<ExchangeCacheTypes::HeightGroupingTypesDescriptor> {};
}}
