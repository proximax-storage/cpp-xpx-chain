/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#pragma once
#include "GlobalStoreCacheTypes.h"
#include "../state/GlobalEntrySerializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

	/// Policy for saving and loading global store cache data.
	struct GlobalStoreCacheStorage
			: public CacheStorageForBasicInsertRemoveCache<GlobalStoreCacheDescriptor>
			, public state::GlobalEntrySerializer
	{};
}}
