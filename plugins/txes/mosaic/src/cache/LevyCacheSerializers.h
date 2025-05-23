/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#pragma once
#include "LevyCacheTypes.h"
#include "src/state/LevyEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"
#include "catapult/cache/IdentifierGroupSerializer.h"

namespace catapult { namespace cache {
		
	/// Primary serializer for levy cache.
	struct LevyEntryPrimarySerializer : public CacheSerializerAdapter<state::LevyEntrySerializer, LevyCacheDescriptor>
	{};
		
	/// Serializer for levy cache without historical information
	struct LevyEntryPatriciaTreeSerializer : public CacheSerializerAdapter<state::LevyEntryNonHistoricalSerializer, LevyCacheDescriptor> {};
	
	/// Serializer for levy cache height grouped elements.
	struct LevyHeightGroupingSerializer : public IdentifierGroupSerializer<LevyCacheTypes::HeightGroupingTypesDescriptor> {};
}}
