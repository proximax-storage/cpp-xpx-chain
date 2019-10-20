/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OfferDeadlineCacheTypes.h"
#include "src/state/OfferDeadlineEntrySerializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

	/// Policy for saving and loading offer deadline cache data.
	struct OfferDeadlineCacheStorage
			: public CacheStorageForBasicInsertRemoveCache<OfferDeadlineCacheDescriptor>
			, public state::OfferDeadlineEntrySerializer {
		/// Loads \a entry into \a cacheDelta.
		static void LoadInto(const ValueType& entry, DestinationType& cacheDelta);
	};
}}
