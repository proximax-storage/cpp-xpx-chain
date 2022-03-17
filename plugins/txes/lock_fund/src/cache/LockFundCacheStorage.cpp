/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "LockFundCacheDelta.h"
#include "LockFundCacheStorage.h"

namespace catapult { namespace cache {

		/// Policy for saving and loading lock fund cache data.
		void LockFundCacheStorage::LoadInto(const ValueType& heightRecord, DestinationType& cacheDelta)
		{
			cacheDelta.insert(heightRecord);
		}
}}
