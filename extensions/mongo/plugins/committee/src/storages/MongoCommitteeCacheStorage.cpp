/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoCommitteeCacheStorage.h"
#include "src/mappers/CommitteeEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/committee/src/cache/CommitteeCache.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct CommitteeCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::CommitteeCacheDescriptor> {
			static constexpr const char* Collection_Name = "harvesters";
			static constexpr const char* Id_Property_Name = "harvester.key";

			static auto MapToMongoId(const KeyType& key) {
				return mappers::ToBinary(key);
			}

			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier networkIdentifier) {
				return plugins::ToDbModel(entry, model::PublicKeyToAddress(entry.key(), networkIdentifier));
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToCommitteeEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(Committee, CommitteeCacheTraits)
}}}
