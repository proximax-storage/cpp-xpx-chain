/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ViewSequenceCacheTypes.h"
#include "src/state/ViewSequenceEntrySerializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

	/// Policy for saving and loading view sequence cache data.
	struct ViewSequenceCacheStorage
			: public CacheStorageForBasicInsertRemoveCache<ViewSequenceCacheDescriptor>
			, public state::ViewSequenceEntrySerializer {
		/// Loads \a entry into \a cacheDelta.
		static void LoadInto(const ValueType& entry, DestinationType& cacheDelta);
	};
}}
