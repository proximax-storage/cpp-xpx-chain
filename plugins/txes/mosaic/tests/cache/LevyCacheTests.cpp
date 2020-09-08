/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/LevyTestUtils.h"
#include "tests/test/cache/CacheBasicTests.h"

namespace catapult { namespace cache {

#define TEST_CLASS LevyCacheTests
	
	// region mixin traits based tests
	
	namespace {
		struct LevyCacheMixinTraits {
			class CacheType : public LevyCache {
			public:
				CacheType() : LevyCache(CacheConfiguration(), config::CreateMockConfigurationHolder())
				{}
			};
			
			using IdType = MosaicId;
			using ValueType = state::LevyEntry;
			
			static uint8_t GetRawId(const IdType& id) {
				return static_cast<uint8_t>(id.unwrap());
			}
			
			static IdType GetId(const ValueType& entry) {
				return entry.mosaicId();
			}
			
			static IdType MakeId(uint8_t id) {
				return IdType(id);
			}
			
			static ValueType CreateWithId(uint8_t id) {
				auto levy = test::CreateValidMosaicLevy();
				return state::LevyEntry(MakeId(id), levy);
			}
		};
	}
	
	DEFINE_CACHE_CONTAINS_TESTS(LevyCacheMixinTraits, ViewAccessor, _View)
	DEFINE_CACHE_CONTAINS_TESTS(LevyCacheMixinTraits, DeltaAccessor, _Delta)
	
	DEFINE_CACHE_ITERATION_TESTS(LevyCacheMixinTraits, ViewAccessor, _View)
	
	DEFINE_CACHE_ACCESSOR_TESTS(LevyCacheMixinTraits, ViewAccessor, MutableAccessor, _ViewMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(LevyCacheMixinTraits, ViewAccessor, ConstAccessor, _ViewConst)
	DEFINE_CACHE_ACCESSOR_TESTS(LevyCacheMixinTraits, DeltaAccessor, MutableAccessor, _DeltaMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(LevyCacheMixinTraits, DeltaAccessor, ConstAccessor, _DeltaConst)
	
	DEFINE_CACHE_MUTATION_TESTS(LevyCacheMixinTraits, DeltaAccessor, _Delta)
	
	DEFINE_CACHE_BASIC_TESTS(LevyCacheMixinTraits,)
	
	// endregion
}}
