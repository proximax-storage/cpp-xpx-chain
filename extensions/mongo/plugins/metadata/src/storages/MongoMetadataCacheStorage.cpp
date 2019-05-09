/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoMetadataCacheStorage.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "src/mappers/MetadataEntryMapper.h"
#include "plugins/txes/metadata/src/cache/MetadataCache.h"
#include "plugins/txes/metadata/src/cache/MetadataCacheTypes.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct MetadataCacheTraits {
			static constexpr auto Collection_Name = "metadatas";
			static constexpr auto Id_Property_Name = "metadata.metadataId";

			using CacheType = cache::MetadataCacheDescriptor::CacheType;
			using CacheDeltaType = cache::MetadataCacheDescriptor::CacheDeltaType;
			using KeyType = std::vector<uint8_t>;
			using ModelType = cache::MetadataCacheDescriptor::ValueType;

			static auto GetId(const ModelType& metadata) {
				return metadata.raw();
			}

			static auto MapToMongoId(const KeyType& raw) {
				return mappers::ToBinary(raw.data(), raw.size());
			}

			static auto MapToMongoDocument(const ModelType& metadataEntry, model::NetworkIdentifier) {
				return plugins::ToDbModel(metadataEntry);
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToMetadataEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(Metadata, MetadataCacheTraits)
}}}
