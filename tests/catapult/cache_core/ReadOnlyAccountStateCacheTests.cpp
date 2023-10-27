/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "catapult/cache_core/ReadOnlyAccountStateCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS ReadOnlyAccountStateCacheTests

	// region cache properties

	namespace {
		auto CreateConfigHolder() {
			test::MutableBlockchainConfiguration config;
			config.Network.ImportanceGrouping = 543;
			config.Network.MinHarvesterBalance = Amount(std::numeric_limits<Amount::ValueType>::max());
			return config::CreateMockConfigurationHolder(config.ToConst());
		}

		auto DefaultOptions() {
			return AccountStateCacheTypes::Options{
				CreateConfigHolder(), model::NetworkIdentifier::Mijin_Test, MosaicId(1111), MosaicId(2222) };
		}
	}

	TEST(TEST_CLASS, NetworkIdentifierIsExposed) {
		// Arrange:
		auto networkIdentifier = static_cast<model::NetworkIdentifier>(19);
		auto options = DefaultOptions();
		options.NetworkIdentifier = networkIdentifier;
		AccountStateCache originalCache(CacheConfiguration(), options);

		// Act + Assert:
		EXPECT_EQ(networkIdentifier, ReadOnlyAccountStateCache(*originalCache.createView(Height{0})).networkIdentifier());
		EXPECT_EQ(networkIdentifier, ReadOnlyAccountStateCache(*originalCache.createDelta(Height{0})).networkIdentifier());
	}

	TEST(TEST_CLASS, ImportanceGroupingIsExposed) {
		// Arrange:
		auto pConfigHolder = CreateConfigHolder();
		const_cast<model::NetworkConfiguration&>(pConfigHolder->Config().Network).ImportanceGrouping = 123;
		auto options = DefaultOptions();
		options.ConfigHolderPtr = pConfigHolder;
		AccountStateCache originalCache(CacheConfiguration(), options);

		// Act + Assert:
		EXPECT_EQ(123u, ReadOnlyAccountStateCache(*originalCache.createView(Height{0})).importanceGrouping());
		EXPECT_EQ(123u, ReadOnlyAccountStateCache(*originalCache.createDelta(Height{0})).importanceGrouping());
	}

	TEST(TEST_CLASS, HarvestingMosaicIdIsExposed) {
		// Arrange:
		auto options = DefaultOptions();
		options.HarvestingMosaicId = MosaicId(11229988);
		AccountStateCache originalCache(CacheConfiguration(), options);

		// Act + Assert:
		EXPECT_EQ(MosaicId(11229988), ReadOnlyAccountStateCache(*originalCache.createView(Height{0})).harvestingMosaicId());
		EXPECT_EQ(MosaicId(11229988), ReadOnlyAccountStateCache(*originalCache.createDelta(Height{0})).harvestingMosaicId());
	}

	// endregion

	namespace {
		struct AccountStateCacheByAddressTraits {
			static Address CreateKey(uint8_t tag) {
				return { { static_cast<uint8_t>(tag * tag) } };
			}

			static Address GetKey(const state::AccountState& accountState) {
				return accountState.Address;
			}
		};

		struct AccountStateCacheByKeyTraits {
			static Key CreateKey(uint8_t tag) {
				return { { static_cast<uint8_t>(tag * tag) } };
			}

			static Key GetKey(const state::AccountState& accountState) {
				return accountState.PublicKey;
			}
		};
	}

#define ACCOUNT_KEY_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_ByAddress) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<AccountStateCacheByAddressTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_ByKey) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<AccountStateCacheByKeyTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	ACCOUNT_KEY_BASED_TEST(ReadOnlyViewOnlyContainsCommittedElements) {
		// Arrange:
		AccountStateCache cache(CacheConfiguration(), DefaultOptions());
		{
			auto cacheDelta = cache.createDelta(Height{0});
			cacheDelta->addAccount(TTraits::CreateKey(1), Height(123)); // committed
			cache.commit();
			cacheDelta->addAccount(TTraits::CreateKey(2), Height(123)); // uncommitted
		}

		// Act:
		auto cacheView = cache.createView(Height{0});
		ReadOnlyAccountStateCache readOnlyCache(*cacheView);

		// Assert:
		EXPECT_EQ(1u, readOnlyCache.size());
		EXPECT_TRUE(readOnlyCache.contains(TTraits::CreateKey(1)));
		EXPECT_FALSE(readOnlyCache.contains(TTraits::CreateKey(2)));
		EXPECT_FALSE(readOnlyCache.contains(TTraits::CreateKey(3)));
	}

	ACCOUNT_KEY_BASED_TEST(ReadOnlyDeltaContainsBothCommittedAndUncommittedElements) {
		// Arrange:
		AccountStateCache cache(CacheConfiguration(), DefaultOptions());
		auto cacheDelta = cache.createDelta(Height{0});
		cacheDelta->addAccount(TTraits::CreateKey(1), Height(123)); // committed
		cache.commit();
		cacheDelta->addAccount(TTraits::CreateKey(2), Height(123)); // uncommitted

		// Act:
		ReadOnlyAccountStateCache readOnlyCache(*cacheDelta);

		// Assert:
		EXPECT_EQ(2u, readOnlyCache.size());
		EXPECT_TRUE(readOnlyCache.contains(TTraits::CreateKey(1)));
		EXPECT_TRUE(readOnlyCache.contains(TTraits::CreateKey(2)));
		EXPECT_FALSE(readOnlyCache.contains(TTraits::CreateKey(3)));
	}

	ACCOUNT_KEY_BASED_TEST(ReadOnlyViewOnlyCanAccessCommittedElementsViaGet) {
		// Arrange:
		AccountStateCache cache(CacheConfiguration(), DefaultOptions());
		{
			auto cacheDelta = cache.createDelta(Height{0});
			cacheDelta->addAccount(TTraits::CreateKey(1), Height(123)); // committed;
			cache.commit();
			cacheDelta->addAccount(TTraits::CreateKey(2), Height(123)); // uncommitted
		}

		// Act:
		auto cacheView = cache.createView(Height{0});
		ReadOnlyAccountStateCache readOnlyCache(*cacheView);

		// Assert:
		EXPECT_EQ(1u, readOnlyCache.size());
		auto keyIter1 = readOnlyCache.find(TTraits::CreateKey(1));
		auto keyIter2 = readOnlyCache.find(TTraits::CreateKey(2));
		auto keyIter3 = readOnlyCache.find(TTraits::CreateKey(3));
		EXPECT_EQ(TTraits::CreateKey(1), TTraits::GetKey(keyIter1.get()));
		EXPECT_THROW(keyIter2.get(), catapult_invalid_argument);
		EXPECT_THROW(keyIter3.get(), catapult_invalid_argument);
	}

	ACCOUNT_KEY_BASED_TEST(ReadOnlyDeltaCanAccessBothCommittedAndUncommittedElementsViaGet) {
		// Arrange:
		AccountStateCache cache(CacheConfiguration(), DefaultOptions());
		auto cacheDelta = cache.createDelta(Height{0});
		cacheDelta->addAccount(TTraits::CreateKey(1), Height(123)); // committed
		cache.commit();
		cacheDelta->addAccount(TTraits::CreateKey(2), Height(123)); // uncommitted

		// Act:
		ReadOnlyAccountStateCache readOnlyCache(*cacheDelta);

		// Assert:
		EXPECT_EQ(2u, readOnlyCache.size());
		auto keyIter1 = readOnlyCache.find(TTraits::CreateKey(1));
		auto keyIter2 = readOnlyCache.find(TTraits::CreateKey(2));
		auto keyIter3 = readOnlyCache.find(TTraits::CreateKey(3));
		EXPECT_EQ(TTraits::CreateKey(1), TTraits::GetKey(keyIter1.get()));
		EXPECT_EQ(TTraits::CreateKey(2), TTraits::GetKey(keyIter2.get()));
		EXPECT_THROW(keyIter3.get(), catapult_invalid_argument);
	}

	ACCOUNT_KEY_BASED_TEST(ReadOnlyViewOnlyCanAccessCommittedElementsViaTryGet) {
		// Arrange:
		AccountStateCache cache(CacheConfiguration(), DefaultOptions());
		{
			auto cacheDelta = cache.createDelta(Height{0});
			cacheDelta->addAccount(TTraits::CreateKey(1), Height(123)); // committed;
			cache.commit();
			cacheDelta->addAccount(TTraits::CreateKey(2), Height(123)); // uncommitted
		}

		// Act:
		auto cacheView = cache.createView(Height{0});
		ReadOnlyAccountStateCache readOnlyCache(*cacheView);

		// Assert:
		EXPECT_EQ(1u, readOnlyCache.size());
		auto keyIter1 = readOnlyCache.find(TTraits::CreateKey(1));
		auto keyIter2 = readOnlyCache.find(TTraits::CreateKey(2));
		auto keyIter3 = readOnlyCache.find(TTraits::CreateKey(3));
		EXPECT_EQ(TTraits::CreateKey(1), TTraits::GetKey(*keyIter1.tryGet()));
		EXPECT_FALSE(!!keyIter2.tryGet());
		EXPECT_FALSE(!!keyIter3.tryGet());
	}

	ACCOUNT_KEY_BASED_TEST(ReadOnlyDeltaCanAccessBothCommittedAndUncommittedElementsViaTryGet) {
		// Arrange:
		AccountStateCache cache(CacheConfiguration(), DefaultOptions());
		auto cacheDelta = cache.createDelta(Height{0});
		cacheDelta->addAccount(TTraits::CreateKey(1), Height(123)); // committed
		cache.commit();
		cacheDelta->addAccount(TTraits::CreateKey(2), Height(123)); // uncommitted

		// Act:
		ReadOnlyAccountStateCache readOnlyCache(*cacheDelta);

		// Assert:
		EXPECT_EQ(2u, readOnlyCache.size());
		auto keyIter1 = readOnlyCache.find(TTraits::CreateKey(1));
		auto keyIter2 = readOnlyCache.find(TTraits::CreateKey(2));
		auto keyIter3 = readOnlyCache.find(TTraits::CreateKey(3));
		EXPECT_EQ(TTraits::CreateKey(1), TTraits::GetKey(*keyIter1.tryGet()));
		EXPECT_EQ(TTraits::CreateKey(2), TTraits::GetKey(*keyIter2.tryGet()));
		EXPECT_FALSE(!!keyIter3.tryGet());
	}
}}
