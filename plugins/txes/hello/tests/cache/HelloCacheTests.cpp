/**
*** FOR TRAINING PURPOSES ONLY
**/

#include "src/cache/HelloCache.h"
#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/CacheMixinsTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"
#include "tests/test/HelloTestUtils.h"

namespace catapult { namespace cache {

#define TEST_CLASS HelloCacheTests

        // region mixin traits based tests

        namespace {
            struct HelloCacheMixinTraits {
                class CacheType : public HelloCache {
                public:
                    CacheType() : HelloCache(CacheConfiguration())
                    {}
                };

                using IdType = Height;
                using ValueType = state::HelloEntry;

                static uint8_t GetRawId(const IdType& id) {
                    return utils::checked_cast<uint64_t, uint8_t>(id.unwrap());
                }

                static IdType GetId(const ValueType& entry) {
                    return entry.height();
                }

                static IdType MakeId(uint8_t id) {
                    return IdType{ id };
                }

                static ValueType CreateWithId(uint8_t id) {
                    return state::HelloEntry(MakeId(id));
                }
            };

            struct HelloEntryModificationPolicy : public test::DeltaRemoveInsertModificationPolicy {
                static void Modify(HelloCacheDelta& delta, const state::HelloEntry& entry) {
                    auto& entryFromCache = delta.find(entry.height()).get();
                    entryFromCache.setBlockchainVersion(BlockchainVersion{entryFromCache.blockChainVersion().unwrap() + 1u});
                }
            };
        }

        DEFINE_CACHE_CONTAINS_TESTS(HelloCacheMixinTraits, ViewAccessor, _View)
        DEFINE_CACHE_CONTAINS_TESTS(HelloCacheMixinTraits, DeltaAccessor, _Delta)

        DEFINE_CACHE_ITERATION_TESTS(HelloCacheMixinTraits, ViewAccessor, _View)

        DEFINE_CACHE_ACCESSOR_TESTS(HelloCacheMixinTraits, ViewAccessor, MutableAccessor, _ViewMutable)
        DEFINE_CACHE_ACCESSOR_TESTS(HelloCacheMixinTraits, ViewAccessor, ConstAccessor, _ViewConst)
        DEFINE_CACHE_ACCESSOR_TESTS(HelloCacheMixinTraits, DeltaAccessor, MutableAccessor, _DeltaMutable)
        DEFINE_CACHE_ACCESSOR_TESTS(HelloCacheMixinTraits, DeltaAccessor, ConstAccessor, _DeltaConst)

        DEFINE_CACHE_MUTATION_TESTS(HelloCacheMixinTraits, DeltaAccessor, _Delta)

        DEFINE_DELTA_ELEMENTS_MIXIN_CUSTOM_TESTS(HelloCacheMixinTraits, HelloEntryModificationPolicy, _Delta)

        DEFINE_CACHE_BASIC_TESTS(HelloCacheMixinTraits,)

        // endregion

        // region custom tests

        TEST(TEST_CLASS, InteractionValuesCanBeChangedLater) {
            // Arrange:
            HelloCacheMixinTraits::CacheType cache;
            auto key = test::GenerateKeys(1);;

            // - insert single account key
            {

                auto delta = cache.createDelta();
                delta->insert(state::HelloEntry(key[0]));
                cache.commit();
            }

            // Sanity:
            EXPECT_EQ(1, cache.createView()->size());

            // Act:
            {
                auto delta = cache.createDelta();
                auto& entry = delta->find(key).get();
                entry->setMessageCount(3);
                cache.commit();
            }

            // Assert:
            auto view = cache.createView();
            const auto& entry = view->find(key).get();
            EXPECT_EQ(3, entry.getMessageCount());
        }

        // endregion
    }}
