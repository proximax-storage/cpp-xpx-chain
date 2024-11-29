/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ExchangeCacheTypes.h"
#include "plugins/txes/exchange/src/state/ExchangeEntrySerializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

	/// Policy for saving and loading exchange cache data.
	struct ExchangeCacheStorage
			: public CacheStorageForBasicInsertRemoveCache<ExchangeCacheDescriptor>
			, public state::ExchangeEntrySerializer {
		/// Loads \a entry into \a cacheDelta.
		static void LoadInto(const ValueType& entry, DestinationType& cacheDelta);
	};
}}
