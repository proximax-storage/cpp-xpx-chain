/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "src/state/LockFundRecordSerializer.h"
#include "LockFundCacheTypes.h"
#include "catapult/cache/CacheStorageInclude.h"
namespace catapult { namespace cache {

		/// Policy for saving and loading lock fund cache data.
		struct LockFundCacheStorage
			: public CacheStorageForBasicInsertRemoveCache<LockFundCacheDescriptor>
						, public state::LockFundRecordSerializer<state::LockFundHeightIndexDescriptor>{

			/// Loads \a recordGroup into \a cacheDelta.
			static void LoadInto(const ValueType& history, DestinationType& cacheDelta);
		};
}}
