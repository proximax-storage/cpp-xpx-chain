/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoDriveCacheStorage.h"
#include "src/mappers/DriveEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/service/src/cache/DriveCache.h"
#include "catapult/model/Address.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct DriveCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::DriveCacheDescriptor> {
			static constexpr const char* Collection_Name = "drives";
			static constexpr const char* Id_Property_Name = "drive.multisig";

			static auto MapToMongoId(const KeyType& key) {
				return mappers::ToBinary(key);
			}

			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier networkIdentifier) {
				return plugins::ToDbModel(entry, model::PublicKeyToAddress(entry.key(), networkIdentifier));
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToDriveEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(Drive, DriveCacheTraits)
}}}
