/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LPCacheStorage.h"
#include "LPCacheDelta.h"

namespace catapult { namespace cache {

	void LPCacheStorage::LoadInto(const ValueType& entry, DestinationType& cacheDelta) {
		cacheDelta.insert(entry);
	}
}}
