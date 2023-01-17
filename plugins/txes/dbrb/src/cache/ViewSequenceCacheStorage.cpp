/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ViewSequenceCacheStorage.h"
#include "ViewSequenceCacheDelta.h"

namespace catapult { namespace cache {

	void ViewSequenceCacheStorage::LoadInto(const ValueType& entry, DestinationType& cacheDelta) {
		cacheDelta.insert(entry);
	}
}}
