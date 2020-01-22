/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoDownloadCacheStorage.h"
#include "src/mappers/DownloadEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/service/src/cache/DownloadCache.h"
#include "catapult/model/Address.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct DownloadCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::DownloadCacheDescriptor> {
			static constexpr const char* Collection_Name = "downloads";
			static constexpr const char* Id_Property_Name = "downloadInfo.driveKey";

			static auto MapToMongoId(const KeyType& key) {
				return mappers::ToBinary(key);
			}

			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier networkIdentifier) {
				return plugins::ToDbModel(entry, model::PublicKeyToAddress(entry.driveKey(), networkIdentifier));
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToDownloadEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(Download, DownloadCacheTraits)
}}}
