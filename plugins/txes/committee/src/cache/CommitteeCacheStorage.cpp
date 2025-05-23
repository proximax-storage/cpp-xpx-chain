/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommitteeCacheStorage.h"
#include "CommitteeCacheDelta.h"

namespace catapult { namespace cache {

	void CommitteeCacheStorage::LoadInto(const ValueType& entry, DestinationType& cacheDelta) {
		cacheDelta.insert(entry);
	}
}}
