/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/ExchangeCache.h"
#include "tests/test/ExchangeTestUtils.h"
#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/CacheMixinsTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"

namespace catapult { namespace cache {

#define TEST_CLASS ExchangeCacheTests

	// region mixin traits based tests

	namespace {
		struct ExchangeCacheMixinTraits {
			class CacheType : public ExchangeCache {
			public:
				CacheType() : ExchangeCache(CacheConfiguration(), config::CreateMockConfigurationHolder())
				{}
			};

			using IdType = Key;
			using ValueType = state::ExchangeEntry;

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
				return test::CreateExchangeEntry(0, 0, MakeId(id));
			}
		};

		struct ExchangeEntryModificationPolicy : public test::DeltaRemoveInsertModificationPolicy {
			static void Modify(ExchangeCacheDelta& delta, const state::ExchangeEntry& entry) {
				auto iter = delta.find(entry.owner());
				auto& entryFromCache = iter.get();
				entryFromCache.buyOffers().emplace(test::GenerateRandomValue<MosaicId>(), state::BuyOffer{test::GenerateOffer(), test::GenerateRandomValue<Amount>()});
				entryFromCache.sellOffers().emplace(test::GenerateRandomValue<MosaicId>(), state::SellOffer{test::GenerateOffer()});
			}
		};
	}

	DEFINE_CACHE_CONTAINS_TESTS(ExchangeCacheMixinTraits, ViewAccessor, _View)
	DEFINE_CACHE_CONTAINS_TESTS(ExchangeCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_CACHE_ITERATION_TESTS(ExchangeCacheMixinTraits, ViewAccessor, _View)

	DEFINE_CACHE_ACCESSOR_TESTS(ExchangeCacheMixinTraits, ViewAccessor, MutableAccessor, _ViewMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(ExchangeCacheMixinTraits, ViewAccessor, ConstAccessor, _ViewConst)
	DEFINE_CACHE_ACCESSOR_TESTS(ExchangeCacheMixinTraits, DeltaAccessor, MutableAccessor, _DeltaMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(ExchangeCacheMixinTraits, DeltaAccessor, ConstAccessor, _DeltaConst)

	DEFINE_CACHE_MUTATION_TESTS(ExchangeCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_DELTA_ELEMENTS_MIXIN_CUSTOM_TESTS(ExchangeCacheMixinTraits, ExchangeEntryModificationPolicy, _Delta)

	DEFINE_CACHE_BASIC_TESTS(ExchangeCacheMixinTraits,)

	// endregion

	// region custom tests

	TEST(TEST_CLASS, InteractionValuesCanBeChangedLater) {
		// Arrange:
		ExchangeCacheMixinTraits::CacheType cache;
		auto key = test::GenerateRandomByteArray<Key>();
		auto offer = test::GenerateOffer();

		// - insert single account key
		{

			auto delta = cache.createDelta(Height(1));
			delta->insert(test::CreateExchangeEntry(0, 0, key));
			cache.commit();
		}

		// Sanity:
		EXPECT_EQ(1, cache.createView(Height(1))->size());

		// Act:
		{
			auto delta = cache.createDelta(Height(1));
			auto& entry = delta->find(key).get();
			entry.buyOffers().emplace(MosaicId(1), state::BuyOffer{offer, Amount(100)});
			entry.sellOffers().emplace(MosaicId(2), state::SellOffer{offer});
			cache.commit();
		}

		// Assert:
		auto view = cache.createView(Height{0});
		const auto& entry = view->find(key).get();
		EXPECT_EQ(1, entry.buyOffers().size());
		test::AssertOffer(offer, dynamic_cast<const state::OfferBase&>(entry.buyOffers().at(MosaicId(1))));
		EXPECT_EQ(Amount(100), entry.buyOffers().at(MosaicId(1)).ResidualCost);
		EXPECT_EQ(1, entry.sellOffers().size());
		test::AssertOffer(offer, dynamic_cast<const state::OfferBase&>(entry.sellOffers().at(MosaicId(2))));
	}

	TEST(TEST_CLASS, ExpiryHeightCanBeAdded) {
		// Arrange:
		Height height(10);
		ExchangeCacheMixinTraits::CacheType cache;
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
		ExchangeCacheMixinTraits::CacheType cache;
		auto key = test::GenerateRandomByteArray<Key>();
		auto delta = cache.createDelta(Height(1));

		// Act:
		delta->addExpiryHeight(key, state::ExchangeEntry::Invalid_Expiry_Height);

		// Assert:
		auto keys = delta->expiringOfferOwners(state::ExchangeEntry::Invalid_Expiry_Height);
		ASSERT_EQ(0u, keys.size());
	}

	TEST(TEST_CLASS, ExpiryHeightCanBeRemoved) {
		// Arrange:
		Height height(10);
		ExchangeCacheMixinTraits::CacheType cache;
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
