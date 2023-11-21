/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "plugins/txes/lock_shared/tests/cache/LockInfoCacheStorageTests.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/DownloadCacheTestUtils.h"

namespace catapult { namespace cache {

#define TEST_CLASS DownloadCacheStorageTests

	namespace {
		struct DownloadStorageTraits : public test::BasicDownloadTestTraits {
			using StorageType = cache::DownloadCacheStorage;
			class CacheType : public test::BasicDownloadTestTraits::CacheType {
			public:
				CacheType(const CacheConfiguration& config) : test::BasicDownloadTestTraits::CacheType(config, config::CreateMockConfigurationHolder())
				{}
			};
		};
	}

	DEFINE_LOCK_INFO_CACHE_STORAGE_TESTS(DownloadStorageTraits)
}}
