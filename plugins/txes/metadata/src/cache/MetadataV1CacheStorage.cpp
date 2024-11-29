/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataV1CacheStorage.h"
#include "MetadataV1CacheDelta.h"

namespace catapult { namespace cache {

	void MetadataV1CacheStorage::LoadInto(const ValueType& metadata, DestinationType& cacheDelta) {
		cacheDelta.insert(metadata);
	}
}}
