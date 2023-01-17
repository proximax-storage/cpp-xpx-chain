/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ViewSequenceCacheTypes.h"
#include "src/state/MessageHashEntrySerializer.h"
#include "src/state/ViewSequenceEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"

namespace catapult { namespace cache {

	/// Primary serializer for view sequence cache.
	struct ViewSequenceEntryPrimarySerializer : public CacheSerializerAdapter<state::ViewSequenceEntrySerializer, ViewSequenceCacheDescriptor>
	{};

	/// Serializer for message hash elements.
	struct MessageHashEntrySerializer : public CacheSerializerAdapter<state::MessageHashEntrySerializer, ViewSequenceCacheTypes::MessageHashTypesDescriptor>
	{};
}}
