/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoSuperContractCacheStorage.h"
#include "src/mappers/SuperContractEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "src/cache/SuperContractCache.h"
#include "catapult/model/Address.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct SuperContractCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::SuperContractCacheDescriptor> {
			static constexpr const char* Collection_Name = "supercontracts";
			static constexpr const char* Id_Property_Name = "supercontract.multisig";

			static auto MapToMongoId(const KeyType& key) {
				return mappers::ToBinary(key);
			}

			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier networkIdentifier) {
				return plugins::ToDbModel(entry, model::PublicKeyToAddress(entry.key(), networkIdentifier));
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToSuperContractEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(SuperContract, SuperContractCacheTraits)
}}}
