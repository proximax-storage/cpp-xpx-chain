/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoFileCacheStorage.h"
#include "src/mappers/FileEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/service/src/cache/FileCache.h"
#include "catapult/model/Address.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct FileCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::FileCacheDescriptor> {
			static constexpr const char* Collection_Name = "files";
			static constexpr const char* Id_Property_Name = "file.key";

			static auto MapToMongoId(const KeyType& key) {
				return mappers::ToBinary(key);
			}

			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier) {
				return plugins::ToDbModel(entry);
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToFileEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(File, FileCacheTraits)
}}}
