/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/SdaExchangeCache.h"
#include "tests/test/SdaExchangeTestUtils.h"
#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/CacheMixinsTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"

namespace catapult { namespace cache {

#define TEST_CLASS SdaExchangeCacheTests

	// region mixin traits based tests

	namespace {
		struct SdaExchangeCacheMixinTraits {
			class CacheType : public SdaExchangeCache {
			public:
				CacheType() : SdaExchangeCache(CacheConfiguration(), config::CreateMockConfigurationHolder())
				{}
			};

			using IdType = Key;
			using ValueType = state::SdaExchangeEntry;

			static uint8_t GetRawId(const IdType& id) {
				return id[0];
			}

			static IdType GetId(const ValueType& entry) {
				return entry.owner();
			}

			static IdType MakeId(uint8_t id) {
				return IdType{ { id } };
			}

			static ValueType CreateWithId(uint8_t id) {
				return test::CreateSdaExchangeEntry(0, 0, MakeId(id));
			}
		};

		struct SdaExchangeEntryModificationPolicy : public test::DeltaRemoveInsertModificationPolicy {
			static void Modify(SdaExchangeCacheDelta& delta, const state::SdaExchangeEntry& entry) {
				auto iter = delta.find(entry.owner());
				auto& entryFromCache = iter.get();
				entryFromCache.sdaOfferBalances().emplace(state::MosaicsPair{test::GenerateRandomValue<MosaicId>(),test::GenerateRandomValue<MosaicId>()}, test::GenerateSdaOfferBalance());
			}
		};
	}

	DEFINE_CACHE_CONTAINS_TESTS(SdaExchangeCacheMixinTraits, ViewAccessor, _View)
	DEFINE_CACHE_CONTAINS_TESTS(SdaExchangeCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_CACHE_ITERATION_TESTS(SdaExchangeCacheMixinTraits, ViewAccessor, _View)

	DEFINE_CACHE_ACCESSOR_TESTS(SdaExchangeCacheMixinTraits, ViewAccessor, MutableAccessor, _ViewMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(SdaExchangeCacheMixinTraits, ViewAccessor, ConstAccessor, _ViewConst)
	DEFINE_CACHE_ACCESSOR_TESTS(SdaExchangeCacheMixinTraits, DeltaAccessor, MutableAccessor, _DeltaMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(SdaExchangeCacheMixinTraits, DeltaAccessor, ConstAccessor, _DeltaConst)

	DEFINE_CACHE_MUTATION_TESTS(SdaExchangeCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_DELTA_ELEMENTS_MIXIN_CUSTOM_TESTS(SdaExchangeCacheMixinTraits, SdaExchangeEntryModificationPolicy, _Delta)

	DEFINE_CACHE_BASIC_TESTS(SdaExchangeCacheMixinTraits,)

	// endregion

	// region custom tests

	TEST(TEST_CLASS, InteractionValuesCanBeChangedLater) {
		// Arrange:
		SdaExchangeCacheMixinTraits::CacheType cache;
		auto key = test::GenerateRandomByteArray<Key>();
		auto offer = test::GenerateSdaOfferBalance();

		// - insert single account key
		{

			auto delta = cache.createDelta(Height(1));
			delta->insert(test::CreateSdaExchangeEntry(0, 0, key));
			cache.commit();
		}

		// Sanity:
		EXPECT_EQ(1, cache.createView(Height(1))->size());

		// Act:
		{
			auto delta = cache.createDelta(Height(1));
			auto& entry = delta->find(key).get();
			entry.sdaOfferBalances().emplace(state::MosaicsPair{MosaicId(1), MosaicId(2)}, offer);
			cache.commit();
		}

		// Assert:
		auto view = cache.createView(Height{0});
		const auto& entry = view->find(key).get();
		EXPECT_EQ(1, entry.sdaOfferBalances().size());
		test::AssertSdaOfferBalance(offer, entry.sdaOfferBalances().at(state::MosaicsPair{MosaicId(1), MosaicId(2)}));
	}

	TEST(TEST_CLASS, ExpiryHeightCanBeAdded) {
		// Arrange:
		Height height(10);
		SdaExchangeCacheMixinTraits::CacheType cache;
		auto key = test::GenerateRandomByteArray<Key>();
		auto delta = cache.createDelta(Height(1));

		// Act:
		delta->addExpiryHeight(key, height);

		// Assert:
		auto keys = delta->expiringOfferOwners(height);
		ASSERT_EQ(1u, keys.size());
		EXPECT_EQ(key, *keys.begin());
	}

	TEST(TEST_CLASS, ZeroExpiryHeightCannotBeAdded) {
		// Arrange:
		SdaExchangeCacheMixinTraits::CacheType cache;
		auto key = test::GenerateRandomByteArray<Key>();
		auto delta = cache.createDelta(Height(1));

		// Act:
		delta->addExpiryHeight(key, state::SdaExchangeEntry::Invalid_Expiry_Height);

		// Assert:
		auto keys = delta->expiringOfferOwners(state::SdaExchangeEntry::Invalid_Expiry_Height);
		ASSERT_EQ(0u, keys.size());
	}

	TEST(TEST_CLASS, ExpiryHeightCanBeRemoved) {
		// Arrange:
		Height height(10);
		SdaExchangeCacheMixinTraits::CacheType cache;
		auto key = test::GenerateRandomByteArray<Key>();
		auto delta = cache.createDelta(Height(1));
		delta->addExpiryHeight(key, height);

		// Sanity:
		ASSERT_EQ(1u, delta->expiringOfferOwners(height).size());

		// Act:
		delta->removeExpiryHeight(key, height);

		// Assert:
		auto keys = delta->expiringOfferOwners(height);
		ASSERT_EQ(0u, keys.size());
	}

	// endregion
}}
