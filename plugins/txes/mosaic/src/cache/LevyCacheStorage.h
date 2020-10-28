/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#pragma once
#include "LevyCacheTypes.h"
#include "src/state/LevyEntrySerializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {
		
		/// Policy for saving and loading catapult upgrade cache data.
		struct LevyCacheStorage
			: public CacheStorageForBasicInsertRemoveCache<LevyCacheDescriptor>
				, public state::LevyEntrySerializer {
		};
	}}
