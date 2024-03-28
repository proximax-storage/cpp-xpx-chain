/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DriveContractCacheTypes.h"
#include "src/state/DriveContractEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"
#include "catapult/cache/IdentifierGroupSerializer.h"

namespace catapult { namespace cache {

	/// Primary serializer for drive DriveContract cache.
	struct DriveContractEntryPrimarySerializer : public CacheSerializerAdapter<state::DriveContractEntrySerializer, DriveContractCacheDescriptor>
	{};
}}
