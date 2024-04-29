/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BootKeyReplicatorCacheStorage.h"
#include "BootKeyReplicatorCacheDelta.h"

namespace catapult { namespace cache {

	void BootKeyReplicatorCacheStorage::LoadInto(const ValueType& entry, DestinationType& cacheDelta) {
		cacheDelta.insert(entry);
	}
}}
