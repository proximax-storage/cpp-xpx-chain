/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoOperationCacheStorage.h"
#include "src/mappers/OperationEntryMapper.h"
#include "mongo/plugins/lock_shared/src/storages/MongoLockInfoCacheStorageTraits.h"
#include "plugins/txes/operation/src/cache/OperationCache.h"

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct OperationCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::OperationCacheDescriptor> {
			static constexpr auto Collection_Name = "operations";
			static constexpr auto Id_Property_Name = "operation.token";
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(Operation, MongoLockInfoCacheStorageTraits<OperationCacheTraits>)
}}}
