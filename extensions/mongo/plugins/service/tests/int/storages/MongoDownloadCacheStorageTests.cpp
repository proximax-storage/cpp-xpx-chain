/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "src/storages/MongoDownloadCacheStorage.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"
#include "plugins/txes/service/tests/test/ServiceTestUtils.h"
#include "tests/test/ServiceMapperTestUtils.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MongoDownloadCacheStorageTests

	namespace {
		struct DownloadCacheTraits {
			using CacheType = cache::DownloadCache;
			using ModelType = state::DownloadEntry;

			static constexpr auto Collection_Name = "downloads";
			static constexpr auto Network_Id = static_cast<model::NetworkIdentifier>(0x5A);
			static constexpr auto CreateCacheStorage = CreateMongoDownloadCacheStorage;

			static cache::CatapultCache CreateCache() {
				return test::DownloadCacheFactory::Create();
			}

			static ModelType GenerateRandomElement(uint32_t id) {
				Key key;
				std::memcpy(key.data(), &id, sizeof(id));

				auto entry = test::CreateDownloadEntry(key);

				return entry;
			}

			static void Add(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& offerCacheDelta = delta.sub<cache::DownloadCache>();
				offerCacheDelta.insert(entry);
			}

			static void Remove(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& offerCacheDelta = delta.sub<cache::DownloadCache>();
				offerCacheDelta.remove(entry.driveKey());
			}

			static void Mutate(cache::CatapultCacheDelta& delta, ModelType& entry) {
				// update expected
				entry.fileRecipients().clear();

				// update cache
				auto& downloadCacheDelta = delta.sub<cache::DownloadCache>();
				auto& entryFromCache = downloadCacheDelta.find(entry.driveKey()).get();
				entryFromCache.fileRecipients().clear();
			}

			static auto GetFindFilter(const ModelType& entry) {
				return document() << "downloadInfo.driveKey" << mappers::ToBinary(entry.driveKey()) << finalize;
			}

			static void AssertEqual(const ModelType& entry, const bsoncxx::document::view& view) {
				auto address = model::PublicKeyToAddress(entry.driveKey(), Network_Id);
				test::AssertEqualDownloadData(entry, address, view["downloadInfo"].get_document().view());
			}
		};
	}

	DEFINE_FLAT_CACHE_STORAGE_TESTS(DownloadCacheTraits,)
}}}
