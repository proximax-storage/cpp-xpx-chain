/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <tests/test/MosaicTestUtils.h>
#include <tests/test/LevyTestUtils.h>
#include "src/cache/LevyCacheStorage.h"
#include "src/cache/LevyCache.h"
#include "src/model/MosaicLevy.h"
#include "tests/test/cache/CacheStorageTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {
	namespace {
		struct LevyCacheStorageTraits {
			using StorageType = LevyCacheStorage;
			class CacheType : public LevyCache {
			public:
				CacheType() : LevyCache(CacheConfiguration(), config::CreateMockConfigurationHolder())
				{}
			};
			
			static auto CreateId(uint8_t id) {
				return MosaicId(id);
			}
			
			static auto CreateValue(MosaicId id) {
				auto levy = test::CreateValidMosaicLevy();
				return state::LevyEntry(id, levy);
			}
			
			static void AssertEqual(const state::LevyEntry& lhs, const state::LevyEntry& rhs) {
				EXPECT_EQ(lhs.mosaicId(), rhs.mosaicId());
				EXPECT_EQ(lhs.levy()->Type, rhs.levy()->Type);
				EXPECT_EQ(lhs.levy()->Fee, rhs.levy()->Fee);
				EXPECT_EQ(lhs.levy()->Recipient, rhs.levy()->Recipient);
				EXPECT_EQ(lhs.levy()->MosaicId, rhs.levy()->MosaicId);
			}
		};
	}
	
	DEFINE_BASIC_INSERT_REMOVE_CACHE_STORAGE_TESTS(LevyCacheStorageTests, LevyCacheStorageTraits)
}}
