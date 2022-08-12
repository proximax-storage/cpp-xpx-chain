/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "plugins/txes/lock_shared/tests/cache/LockInfoCacheTests.h"
#include "tests/test/OperationTestUtils.h"

namespace catapult { namespace cache {

#define TEST_CLASS OperationCacheTests

	namespace {
		struct OperationTraits : public test::BasicOperationTestTraits {
		public:
			class CacheType : public test::BasicOperationTestTraits::CacheType {
			public:
				CacheType(const CacheConfiguration& config) : test::BasicOperationTestTraits::CacheType(config, config::CreateMockConfigurationHolder())
				{}
			};

		public:
			static void SetStatus(state::OperationEntry& operationEntry, state::LockStatus status) {
				operationEntry.Status = status;
			}
		};
	}

	DEFINE_LOCK_INFO_CACHE_TESTS(LockInfoCacheDeltaElementsMixinTraits<OperationTraits>, LockInfoCacheDeltaModificationPolicy<OperationTraits>,)

	DEFINE_CACHE_PRUNE_TESTS(LockInfoCacheDeltaElementsMixinTraits<OperationTraits>,)
}}
