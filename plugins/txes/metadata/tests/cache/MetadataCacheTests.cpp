/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/MetadataV1Cache.h"
#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/CachePruneTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"

namespace catapult { namespace cache {

#define TEST_CLASS MetadataV1CacheTests

	// region mixin traits based tests

	namespace {
		constexpr auto Max_Rollback_Blocks = 7u;

		struct MetadataCacheMixinTraits {
			class CacheType : public MetadataV1Cache {
			public:
				CacheType() : MetadataV1Cache(CacheConfiguration())
				{}
			};

			using IdType = Hash256;
			using ValueType = state::MetadataV1Entry;

			static uint8_t GetRawId(const IdType& id) {
				return id[0];
			}

			static IdType GetId(const ValueType& entry) {
				return entry.metadataId();
			}

			static IdType MakeId(uint8_t id) {
				return IdType{ { id } };
			}

			static ValueType CreateWithId(uint8_t id) {
				return state::MetadataV1Entry(IdType{ { id } });
			}

			static state::MetadataV1Entry CreateWithIdAndExpiration(uint8_t id, Height height) {
				auto metadataEntry = state::MetadataV1Entry(IdType{ { id } });
				metadataEntry.fields().push_back(state::MetadataV1Field { "Key" , "Value", height });
				return metadataEntry;
			}
		};

		struct MetadataEntryModificationPolicy : public test::DeltaRemoveInsertModificationPolicy {
			static void Modify(MetadataV1CacheDelta& delta, const state::MetadataV1Entry& metadataEntry) {
				auto& metadataFromCache = delta.find(metadataEntry.metadataId()).get();
				metadataFromCache.fields().push_back(state::MetadataV1Field { "Key" , "Value", Height(0) });
			}
		};
	}

	DEFINE_CACHE_CONTAINS_TESTS(MetadataCacheMixinTraits, ViewAccessor, _View)
	DEFINE_CACHE_CONTAINS_TESTS(MetadataCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_CACHE_ITERATION_TESTS(MetadataCacheMixinTraits, ViewAccessor, _View)

	DEFINE_CACHE_ACCESSOR_TESTS(MetadataCacheMixinTraits, ViewAccessor, MutableAccessor, _ViewMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(MetadataCacheMixinTraits, ViewAccessor, ConstAccessor, _ViewConst)
	DEFINE_CACHE_ACCESSOR_TESTS(MetadataCacheMixinTraits, DeltaAccessor, MutableAccessor, _DeltaMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(MetadataCacheMixinTraits, DeltaAccessor, ConstAccessor, _DeltaConst)

	DEFINE_CACHE_PRUNE_TESTS(MetadataCacheMixinTraits,)

	DEFINE_DELTA_ELEMENTS_MIXIN_CUSTOM_TESTS(MetadataCacheMixinTraits, MetadataEntryModificationPolicy, _Delta)

	DEFINE_CACHE_BASIC_TESTS(MetadataCacheMixinTraits,)

	// endregion
}}
