/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/SdaOfferGroupCache.h"
#include "tests/test/SdaExchangeTestUtils.h"
#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/CacheMixinsTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"

namespace catapult { namespace cache {

#define TEST_CLASS SdaOfferGroupCacheTests

    // region mixin traits based tests

    namespace {
        struct SdaOfferGroupCacheMixinTraits {
            class CacheType : public SdaOfferGroupCache {
            public:
                CacheType() : SdaOfferGroupCache(CacheConfiguration(), config::CreateMockConfigurationHolder())
                {}
            };

            using IdType = Hash256;
            using ValueType = state::SdaOfferGroupEntry;

            static uint8_t GetRawId(const IdType& id) {
                return id[0];
            }

            static IdType GetId(const ValueType& entry) {
                return entry.groupHash();
            }

            static IdType MakeId(uint8_t id) {
                return IdType{ { id } };
            }

            static ValueType CreateWithId(uint8_t id) {
                return test::CreateSdaOfferGroupEntry(0, MakeId(id));
            }
        };

        struct SdaOfferGroupEntryModificationPolicy : public test::DeltaRemoveInsertModificationPolicy {
            static void Modify(SdaOfferGroupCacheDelta& delta, const state::SdaOfferGroupEntry& entry) {
                auto iter = delta.find(entry.groupHash());
                auto& entryFromCache = iter.get();
                entryFromCache.sdaOfferGroup().emplace(entry.groupHash(), test::GenerateSdaOfferBasicInfo());
            }
        };
    }

    DEFINE_CACHE_CONTAINS_TESTS(SdaOfferGroupCacheMixinTraits, ViewAccessor, _View)
    DEFINE_CACHE_CONTAINS_TESTS(SdaOfferGroupCacheMixinTraits, DeltaAccessor, _Delta)

    DEFINE_CACHE_ITERATION_TESTS(SdaOfferGroupCacheMixinTraits, ViewAccessor, _View)

    DEFINE_CACHE_ACCESSOR_TESTS(SdaOfferGroupCacheMixinTraits, ViewAccessor, MutableAccessor, _ViewMutable)
    DEFINE_CACHE_ACCESSOR_TESTS(SdaOfferGroupCacheMixinTraits, ViewAccessor, ConstAccessor, _ViewConst)
    DEFINE_CACHE_ACCESSOR_TESTS(SdaOfferGroupCacheMixinTraits, DeltaAccessor, MutableAccessor, _DeltaMutable)
    DEFINE_CACHE_ACCESSOR_TESTS(SdaOfferGroupCacheMixinTraits, DeltaAccessor, ConstAccessor, _DeltaConst)

    DEFINE_CACHE_MUTATION_TESTS(SdaOfferGroupCacheMixinTraits, DeltaAccessor, _Delta)

    DEFINE_DELTA_ELEMENTS_MIXIN_CUSTOM_TESTS(SdaOfferGroupCacheMixinTraits, SdaOfferGroupEntryModificationPolicy, _Delta)

    DEFINE_CACHE_BASIC_TESTS(SdaOfferGroupCacheMixinTraits,)

    // endregion

    // region custom tests

    TEST(TEST_CLASS, InteractionValuesCanBeChangedLater) {
        // Arrange:
        SdaOfferGroupCacheMixinTraits::CacheType cache;
        auto groupHash = test::GenerateRandomByteArray<Hash256>();
        auto offer = test::GenerateSdaOfferBasicInfo(2);

        // - insert single account groupHash
        {

            auto delta = cache.createDelta(Height(1));
            delta->insert(test::CreateSdaOfferGroupEntry(0, groupHash));
            cache.commit();
        }

        // Sanity:
        EXPECT_EQ(1, cache.createView(Height(1))->size());

        // Act:
        {
            auto delta = cache.createDelta(Height(1));
            auto& entry = delta->find(groupHash).get();
            entry.sdaOfferGroup().emplace(groupHash, offer);
            cache.commit();
        }

        // Assert:
        auto view = cache.createView(Height{0});
        const auto& entry = view->find(groupHash).get();
        EXPECT_EQ(1, entry.sdaOfferGroup().size());
        test::AssertSdaOfferBasicInfo(offer, entry.sdaOfferGroup().at(groupHash));
    }

    // endregion
}}
