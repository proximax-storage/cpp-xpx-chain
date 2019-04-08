/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataCacheStorage.h"
#include "MetadataCacheDelta.h"

namespace catapult { namespace cache {

	void MetadataCacheStorage::LoadInto(const ValueType& metadata, DestinationType& cacheDelta) {
		cacheDelta.insert(metadata);
	}
}}
