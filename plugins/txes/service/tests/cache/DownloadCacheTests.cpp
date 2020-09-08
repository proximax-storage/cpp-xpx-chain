/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "plugins/txes/lock_shared/tests/cache/LockInfoCacheTests.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/DownloadCacheTestUtils.h"

namespace catapult { namespace cache {

#define TEST_CLASS DownloadCacheTests

	namespace {
		struct DownloadTraits : public test::BasicDownloadTestTraits {
		public:
			class CacheType : public test::BasicDownloadTestTraits::CacheType {
			public:
				CacheType(const CacheConfiguration& config) : test::BasicDownloadTestTraits::CacheType(config, config::CreateMockConfigurationHolder())
				{}
			};

		public:
			static void SetStatus(state::DownloadEntry& entry, state::LockStatus status) {
				if (state::LockStatus::Used == status)
					entry.Files.clear();
				else if (entry.Files.empty())
					entry.Files.emplace(test::GenerateRandomByteArray<Hash256>(), test::Random());
			}
		};

		struct DownloadCacheDeltaModificationPolicy : public test:: DeltaInsertModificationPolicy {
		public:
			template<typename TDelta, typename TValue>
			static void Modify(TDelta& delta, const TValue& value) {
				delta.find(DownloadTraits::ToKey(value)).get().Files.clear();
			}
		};
	}

	DEFINE_LOCK_INFO_CACHE_TESTS(LockInfoCacheDeltaElementsMixinTraits<DownloadTraits>, DownloadCacheDeltaModificationPolicy,)

	DEFINE_CACHE_PRUNE_TESTS(LockInfoCacheDeltaElementsMixinTraits<DownloadTraits>,)
}}
