/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteeCacheTypes.h"
#include "plugins/txes/committee/src/state/CommitteeEntrySerializers.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

	/// Policy for saving and loading committee cache data.
	struct CommitteeCacheStorage
			: public CacheStorageForBasicInsertRemoveCache<CommitteeCacheDescriptor>
			, public state::CommitteeEntrySerializer {
		/// Loads \a entry into \a cacheDelta.
		static void LoadInto(const ValueType& entry, DestinationType& cacheDelta);
	};
}}
