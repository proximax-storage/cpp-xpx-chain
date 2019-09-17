/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "FileCacheTypes.h"
#include "src/state/FileEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"

namespace catapult { namespace cache {

	/// Primary serializer for file cache.
	struct FileEntryPrimarySerializer : public CacheSerializerAdapter<state::FileEntrySerializer, FileCacheDescriptor>
	{};
}}
