/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataV1CacheTypes.h"
#include "plugins/txes/metadata/src/state/MetadataV1Serializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"
#include "catapult/cache/IdentifierGroupSerializer.h"

namespace catapult { namespace cache {

	/// Primary serializer for metadata cache.
	struct MetadataV1PrimarySerializer
			: public CacheSerializerAdapter<state::MetadataV1Serializer, MetadataV1CacheDescriptor>
	{};

	/// Serializer for metadata cache height grouped elements.
	struct MetadataV1HeightGroupingSerializer : public IdentifierGroupSerializer<MetadataV1CacheTypes::HeightGroupingTypesDescriptor> {};
}}
