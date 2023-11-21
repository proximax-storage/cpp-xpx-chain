/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorCacheStorage.h"
#include "ReplicatorCacheDelta.h"

namespace catapult { namespace cache {

	void ReplicatorCacheStorage::LoadInto(const ValueType& entry, DestinationType& cacheDelta) {
		cacheDelta.insert(entry);
	}
}}
