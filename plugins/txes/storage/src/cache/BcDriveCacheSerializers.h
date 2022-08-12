/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BcDriveCacheTypes.h"
#include "src/state/BcDriveEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"

namespace catapult { namespace cache {

	/// Primary serializer for drive cache.
	struct BcDriveEntryPrimarySerializer : public CacheSerializerAdapter<state::BcDriveEntrySerializer, BcDriveCacheDescriptor>
	{};
}}
