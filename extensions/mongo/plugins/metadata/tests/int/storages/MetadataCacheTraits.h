/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/metadata/tests/test/MetadataCacheTestUtils.h"
#include "src/storages/MongoMetadataCacheStorage.h"
#include "tests/test/MetadataMapperTestUtils.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace test {

    template<typename TMetadataEntryTraits>
    struct MetadataCacheTraits {
        using CacheType = cache::MetadataCache;
        using ModelType = state::MetadataEntry;

        static constexpr auto Collection_Name = "metadatas";
        static constexpr auto Network_Id = static_cast<model::NetworkIdentifier>(0x5A);
        static constexpr auto CreateCacheStorage = mongo::plugins::CreateMongoMetadataCacheStorage;

        static cache::CatapultCache CreateCache(const model::NetworkConfiguration& config) {
            return test::MetadataCacheFactory::Create(config);
        }

        static ModelType GenerateRandomElement(uint32_t) {
            auto buffer = TMetadataEntryTraits::GenerateEntryBuffer(Network_Id);
            auto entry = ModelType{ buffer, TMetadataEntryTraits::MetadataType };

            for (uint32_t i = 0; i < 5; ++i) {
                entry.fields().emplace_back(state::MetadataField{
                        test::GenerateRandomString(15), test::GenerateRandomString(10), Height{0}});
            }

            return entry;
        }

        static void Add(cache::CatapultCacheDelta& delta, const ModelType& entry) {
            auto& metadataCacheDelta = delta.sub<cache::MetadataCache>();
            metadataCacheDelta.insert(entry);
        }

        static void Remove(cache::CatapultCacheDelta& delta, const ModelType& entry) {
            auto& metadataCacheDelta = delta.sub<cache::MetadataCache>();
            metadataCacheDelta.remove(entry.metadataId());
        }

        static void Mutate(cache::CatapultCacheDelta& delta, ModelType& entry) {
            // update expected
            auto key = test::GenerateRandomString(5);
            auto value = test::GenerateRandomString(8);
            entry.fields().emplace_back(state::MetadataField{key, value, Height{0}});

            // update cache
            auto& metadataCacheDelta = delta.sub<cache::MetadataCache>();
            auto& entryFromCache = metadataCacheDelta.find(entry.metadataId()).get();
            entryFromCache.fields().emplace_back(state::MetadataField{key, value, Height{0}});
        }

        static auto GetFindFilter(const ModelType& entry) {
            return document() << "metadata.metadataId" << mongo::mappers::ToBinary(entry.raw().data(), entry.raw().size()) << finalize;
        }

        static void AssertEqual(const ModelType& entry, const bsoncxx::document::view& view) {
            test::AssertEqualMetadataData(entry, view["metadata"].get_document().view());
        }
    };
}}
