/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoViewSequenceCacheStorage.h"
#include "src/mappers/ViewSequenceEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/dbrb/src/cache/ViewSequenceCache.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct ViewSequenceCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::ViewSequenceCacheDescriptor> {
			static constexpr const char* Collection_Name = "viewSequences";
			static constexpr const char* Id_Property_Name = "viewSequence.hash";

			static auto MapToMongoId(const KeyType& key) {
				return mappers::ToBinary(key);
			}

			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier) {
				return plugins::ToDbModel(entry);
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToViewSequenceEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(ViewSequence, ViewSequenceCacheTraits)
}}}
