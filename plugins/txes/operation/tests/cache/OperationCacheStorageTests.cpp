/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/OperationCacheStorage.h"
#include "plugins/txes/lock_shared/tests/cache/LockInfoCacheStorageTests.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/OperationTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS OperationCacheStorageTests

	namespace {
		struct OperationStorageTraits : public test::BasicOperationTestTraits {
			using StorageType = cache::OperationCacheStorage;
			class CacheType : public test::BasicOperationTestTraits::CacheType {
			public:
				CacheType(const CacheConfiguration& config) : test::BasicOperationTestTraits::CacheType(config, config::CreateMockConfigurationHolder())
				{}
			};
		};
	}

	DEFINE_LOCK_INFO_CACHE_STORAGE_TESTS(OperationStorageTraits)
}}
