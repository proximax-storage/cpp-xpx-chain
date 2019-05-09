/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataCacheTypes.h"
#include "plugins/txes/metadata/src/state/MetadataSerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"
#include "catapult/cache/IdentifierGroupSerializer.h"

namespace catapult { namespace cache {

	/// Primary serializer for metadata cache.
	struct MetadataPrimarySerializer
			: public CacheSerializerAdapter<state::MetadataSerializer, MetadataCacheDescriptor>
	{};

	/// Serializer for metadata cache height grouped elements.
	struct MetadataHeightGroupingSerializer : public IdentifierGroupSerializer<MetadataCacheTypes::HeightGroupingTypesDescriptor> {};
}}
