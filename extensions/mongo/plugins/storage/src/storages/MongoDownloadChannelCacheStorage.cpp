/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoDownloadChannelCacheStorage.h"
#include "src/mappers/DownloadChannelEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/storage/src/cache/DownloadChannelCache.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct DownloadChannelCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::DownloadChannelCacheDescriptor> {
			static constexpr const char* Collection_Name = "downloadChannels";
			static constexpr const char* Id_Property_Name = "downloadChannelInfo.id";

			static auto MapToMongoId(const KeyType& key) {
				return mappers::ToBinary(key);
			}

			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier) {
				return plugins::ToDbModel(entry);
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToDownloadChannelEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(DownloadChannel, DownloadChannelCacheTraits)
}}}
