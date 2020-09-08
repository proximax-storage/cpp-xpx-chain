/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/storages/MongoOperationCacheStorage.h"
#include "mongo/plugins/lock_shared/tests/int/storages/MongoLockInfoCacheStorageTestTraits.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"
#include "plugins/txes/operation/tests/test/OperationTestUtils.h"
#include "tests/test/OperationMapperTestUtils.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MongoOperationCacheStorageTests

	namespace {
		struct OperationCacheTraits {
			using CacheType = cache::OperationCache;
			using ModelType = state::OperationEntry;

			static constexpr auto Collection_Name = "operations";
			static constexpr auto Id_Property_Name = "operation.token";
			static constexpr char Doc_Name[] = "operation";

			static constexpr auto CreateCacheStorage = CreateMongoOperationCacheStorage;
			static constexpr auto AssertEqualLockInfoData = test::AssertEqualMongoOperationData;

			static auto GetId(const ModelType& entry) {
				return entry.OperationToken;
			}

			static cache::CatapultCache CreateCache() {
				return test::OperationCacheFactory::Create();
			}

			static ModelType GenerateRandomElement(uint32_t id) {
				return test::BasicOperationTestTraits::CreateLockInfo(Height(id));
			}
		};
	}

	DEFINE_FLAT_CACHE_STORAGE_TESTS(MongoLockInfoCacheStorageTestTraits<OperationCacheTraits>,)
}}}
