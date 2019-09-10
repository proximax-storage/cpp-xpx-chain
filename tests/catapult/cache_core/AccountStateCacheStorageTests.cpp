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

#include "catapult/cache_core/AccountStateCacheStorage.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/model/Address.h"
#include "tests/test/cache/CacheStorageTestUtils.h"
#include "tests/test/core/AccountStateTestUtils.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/nodeps/TestConstants.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS AccountStateCacheStorageTests

	namespace {
		struct AccountStateCacheStorageTraits {
		private:
			static auto CreateConfigHolder() {
				test::MutableBlockchainConfiguration config;
				config.Network.ImportanceGrouping = 543;
				config.Network.MinHarvesterBalance = Amount(std::numeric_limits<Amount::ValueType>::max());
				return config::CreateMockConfigurationHolder(config.ToConst());
			}

		public:
			using StorageType = AccountStateCacheStorage;
			class CacheType : public AccountStateCache {
			public:
				CacheType() : AccountStateCache(CacheConfiguration(), AccountStateCacheTypes::Options{
						CreateConfigHolder(),
						model::NetworkIdentifier::Mijin_Test,
						test::Default_Currency_Mosaic_Id,
						test::Default_Harvesting_Mosaic_Id
					})
				{}
			};

			static auto CreateId(uint8_t id) {
				return Address{ { id } };
			}

			static auto CreateValue(const Address& address) {
				state::AccountState accountState(address, Height(111));
				test::RandomFillAccountData(0, accountState, 123);
				return accountState;
			}

			static void AssertEqual(state::AccountState& lhs, const state::AccountState& rhs) {
				// Arrange: cache automatically optimizes added account state, so update to match expected
				lhs.Balances.optimize(test::Default_Currency_Mosaic_Id);

				// Assert: the loaded cache value is correct
				EXPECT_EQ(123u, rhs.Balances.size());
				test::AssertEqual(lhs, rhs);
			}
		};
	}

	TEST(TEST_CLASS, CanLoadValueIntoCache) {
		// Assert:
		test::BasicInsertRemoveCacheStorageTests<AccountStateCacheStorageTraits>::AssertCanLoadValueIntoCache();
	}

	TEST(TEST_CLASS, CanPurgeNonexistentValueFromCache) {
		// Assert:
		test::BasicInsertRemoveCacheStorageTests<AccountStateCacheStorageTraits>::AssertCanPurgeNonexistentValueFromCache();
	}

	TEST(TEST_CLASS, CanPurgeValueWithAddressFromCache) {
		// Arrange:
		auto publicKey = test::GenerateRandomByteArray<Key>();
		auto address = model::PublicKeyToAddress(publicKey, model::NetworkIdentifier::Mijin_Test);
		auto otherAddress = test::GenerateRandomAddress();

		// - add two accounts one of which will be purged
		AccountStateCacheStorageTraits::CacheType cache;
		{
			auto delta = cache.createDelta(Height{0});
			delta->addAccount(address, Height(111));
			delta->addAccount(otherAddress, Height(111));
			cache.commit();
		}

		// Sanity:
		EXPECT_TRUE(cache.createView(Height{0})->contains(address));
		EXPECT_FALSE(cache.createView(Height{0})->contains(publicKey));
		EXPECT_TRUE(cache.createView(Height{0})->contains(otherAddress));

		// Act:
		{
			state::AccountState accountState(address, Height(111));
			accountState.PublicKey = publicKey;
			accountState.PublicKeyHeight = Height(224);

			auto delta = cache.createDelta(Height{0});
			AccountStateCacheStorageTraits::StorageType::Purge(accountState, *delta);
			cache.commit();
		}

		// Assert:
		EXPECT_FALSE(cache.createView(Height{0})->contains(address));
		EXPECT_FALSE(cache.createView(Height{0})->contains(publicKey));
		EXPECT_TRUE(cache.createView(Height{0})->contains(otherAddress));
	}

	TEST(TEST_CLASS, CanPurgeValueWithAddressAndPublicKeyFromCache) {
		// Arrange:
		auto publicKey = test::GenerateRandomByteArray<Key>();
		auto address = model::PublicKeyToAddress(publicKey, model::NetworkIdentifier::Mijin_Test);
		auto otherAddress = test::GenerateRandomAddress();

		// - add two accounts one of which will be purged
		AccountStateCacheStorageTraits::CacheType cache;
		{
			auto delta = cache.createDelta(Height{0});
			delta->addAccount(address, Height(111));
			delta->addAccount(publicKey, Height(224));
			delta->addAccount(otherAddress, Height(111));
			cache.commit();
		}

		// Sanity:
		EXPECT_TRUE(cache.createView(Height{0})->contains(address));
		EXPECT_TRUE(cache.createView(Height{0})->contains(publicKey));
		EXPECT_TRUE(cache.createView(Height{0})->contains(otherAddress));

		// Act:
		{
			state::AccountState accountState(address, Height(111));
			accountState.PublicKey = publicKey;
			accountState.PublicKeyHeight = Height(224);

			auto delta = cache.createDelta(Height{0});
			AccountStateCacheStorageTraits::StorageType::Purge(accountState, *delta);
			cache.commit();
		}

		// Assert:
		EXPECT_FALSE(cache.createView(Height{0})->contains(address));
		EXPECT_FALSE(cache.createView(Height{0})->contains(publicKey));
		EXPECT_TRUE(cache.createView(Height{0})->contains(otherAddress));
	}
}}
