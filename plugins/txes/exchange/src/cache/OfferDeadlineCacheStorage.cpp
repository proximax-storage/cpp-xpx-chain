/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "OfferDeadlineCacheStorage.h"
#include "OfferDeadlineCacheDelta.h"

namespace catapult { namespace cache {

	void OfferDeadlineCacheStorage::LoadInto(const ValueType& entry, DestinationType& cacheDelta) {
		cacheDelta.insert(entry);
	}
}}
