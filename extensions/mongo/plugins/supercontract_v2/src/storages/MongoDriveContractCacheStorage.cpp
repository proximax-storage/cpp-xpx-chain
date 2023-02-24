/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoDriveContractCacheStorage.h"
#include "src/mappers/DriveContractEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "src/cache/DriveContractCache.h"
#include "catapult/model/Address.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct DriveContractCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::DriveContractCacheDescriptor> {
			static constexpr const char* Collection_Name = "drivecontracts";
			static constexpr const char* Id_Property_Name = "drivecontract.driveKey";

			static auto MapToMongoId(const KeyType& key) {
				return mappers::ToBinary(key);
			}

			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier networkIdentifier) {
				return plugins::ToDbModel(entry);
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToDriveContractEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(DriveContract, DriveContractCacheTraits)
}}}
