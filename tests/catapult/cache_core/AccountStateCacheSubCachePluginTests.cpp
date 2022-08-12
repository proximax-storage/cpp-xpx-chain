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

#include "catapult/cache_core/AccountStateCacheSubCachePlugin.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/cache/SummaryAwareCacheStoragePluginTests.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/mocks/MockMemoryStream.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"

namespace catapult { namespace cache {

#define TEST_CLASS AccountStateCacheSubCachePluginTests

	// region AccountStateCacheSummaryCacheStorage - saveAll / saveSummary

	namespace {
		constexpr auto Harvesting_Mosaic_Id = MosaicId(9876);

		cache::AccountStateCacheTypes::Options CreateAccountStateCacheOptions(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
			const auto& config = pConfigHolder->Config().Immutable;
			return {
				pConfigHolder,
				config.NetworkIdentifier,
				config.CurrencyMosaicId,
				config.HarvestingMosaicId
			};
		}

		std::vector<Address> AddAccountsWithBalances(AccountStateCacheDelta& delta, const std::vector<Amount>& balances) {
			auto addresses = test::GenerateRandomDataVector<Address>(balances.size());
			for (auto i = 0u; i < balances.size(); ++i) {
				delta.addAccount(addresses[i], Height(1));
				auto& accountState = delta.find(addresses[i]).get();
				accountState.Balances.track(Harvesting_Mosaic_Id);
				accountState.Balances.credit(Harvesting_Mosaic_Id, balances[i], Height(2));
			}

			return addresses;
		}

		template<typename TAction>
		void RunCacheStorageTest(Amount minHighValueAccountBalance, TAction action) {
			// Arrange:
			test::MutableBlockchainConfiguration mutableConfig;
			mutableConfig.Immutable.HarvestingMosaicId = Harvesting_Mosaic_Id;
			mutableConfig.Network.MinHarvesterBalance = minHighValueAccountBalance;
			auto config = mutableConfig.ToConst();
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			AccountStateCache cache(CacheConfiguration(), CreateAccountStateCacheOptions(pConfigHolder));
			AccountStateCacheSummaryCacheStorage storage(cache);

			// Act + Assert:
			action(storage, config, cache);
		}

		template<typename TAction>
		void RunSummarySaveTest(Amount minHighValueAccountBalance, size_t numExpectedAccounts, TAction checkAddresses) {
			// Arrange:
			RunCacheStorageTest(minHighValueAccountBalance, [numExpectedAccounts, checkAddresses](
					const auto& storage,
					const auto& config,
					const auto&) {
				auto catapultCache = test::CoreSystemCacheFactory::Create(config);
				auto cacheDelta = catapultCache.createDelta();
				auto& delta = cacheDelta.template sub<AccountStateCache>();
				auto balances = { Amount(1'000'000), Amount(500'000), Amount(750'000), Amount(1'250'000) };
				auto addresses = AddAccountsWithBalances(delta, balances);
				catapultCache.commit(Height{0});

				std::vector<uint8_t> buffer;
				mocks::MockMemoryStream stream(buffer);

				// Act:
				storage.saveSummary(cacheDelta, stream);

				// Assert: all addresses were saved
				ASSERT_EQ(sizeof(VersionType) + sizeof(uint64_t) + numExpectedAccounts * sizeof(Address) +
						  sizeof(uint64_t) + addresses.size() * sizeof(Address), buffer.size());

				auto numAddresses = reinterpret_cast<uint64_t&>(*(buffer.data() + sizeof(VersionType)));
				EXPECT_EQ(numExpectedAccounts, numAddresses);

				model::AddressSet savedAddresses;
				for (auto i = 0u; i < numAddresses; ++i)
					savedAddresses.insert(reinterpret_cast<Address&>(*(buffer.data() + sizeof(VersionType) + sizeof(uint64_t) + i * sizeof(Address))));

				checkAddresses(addresses, savedAddresses);

				// - there was a single flush
				EXPECT_EQ(1u, stream.numFlushes());
			});
		}
	}

	TEST(TEST_CLASS, CannotSaveAll) {
		// Arrange:
		RunCacheStorageTest(Amount(2'000'000), [](const auto& storage, const auto& config, const auto&) {
			auto catapultCache = test::CoreSystemCacheFactory::Create(config);
			auto cacheView = catapultCache.createView();

			std::vector<uint8_t> buffer;
			mocks::MockMemoryStream stream(buffer);

			// Act + Assert:
			EXPECT_THROW(storage.saveAll(cacheView, stream), catapult_invalid_argument);
		});
	}

	TEST(TEST_CLASS, CanSaveSummaryWithZeroHighValueAddresses) {
		// Act:
		RunSummarySaveTest(Amount(2'000'000), 0, [](const auto&, const auto& savedAddresses) {
			// Assert:
			EXPECT_TRUE(savedAddresses.empty());
		});
	}

	TEST(TEST_CLASS, CanSaveSummaryWithSingleHighValueAddress) {
		// Act:
		RunSummarySaveTest(Amount(1'111'111), 1, [](const auto& originalAddresses, const auto& savedAddresses) {
			// Assert:
			EXPECT_EQ(model::AddressSet{ originalAddresses[3] }, savedAddresses);
		});
	}

	TEST(TEST_CLASS, CanSaveSummaryWithMultipleHighValueAddresses) {
		// Act:
		RunSummarySaveTest(Amount(700'000), 3, [](const auto& originalAddresses, const auto& savedAddresses) {
			// Assert:
			EXPECT_EQ((model::AddressSet{ originalAddresses[0], originalAddresses[2], originalAddresses[3] }), savedAddresses);
		});
	}

	// endregion

	// region AccountStateCacheSummaryCacheStorage - loadAll

	namespace {
		void RunSummaryLoadTest(size_t numAccounts) {
			// Arrange:
			auto config = CacheConfiguration();
			AccountStateCache cache(config, CreateAccountStateCacheOptions(config::CreateMockConfigurationHolder()));
			AccountStateCacheSummaryCacheStorage storage(cache);

			size_t numUpdateAddresses = 2 * numAccounts;
			auto highValueAddresses = test::GenerateRandomDataVector<Address>(numAccounts);
			auto addressesToUpdate = test::GenerateRandomDataVector<Address>(numUpdateAddresses);

			std::vector<uint8_t> buffer;
			mocks::MockMemoryStream stream(buffer);
			io::Write32(stream, 1);
			io::Write64(stream, numAccounts);
			stream.write({ reinterpret_cast<const uint8_t*>(highValueAddresses.data()), numAccounts * sizeof(Address) });
			io::Write64(stream, numUpdateAddresses);
			stream.write({ reinterpret_cast<const uint8_t*>(addressesToUpdate.data()), numUpdateAddresses * sizeof(Address) });

			// Act:
			storage.loadAll(stream, 0);

			// Assert: all addresses were saved
			auto view = cache.createView(Height{0});
			EXPECT_EQ(numAccounts, view->highValueAddresses().size());
			EXPECT_EQ(model::AddressSet(highValueAddresses.cbegin(), highValueAddresses.cend()), view->highValueAddresses());
			EXPECT_EQ(numUpdateAddresses, view->addressesToUpdate().size());
			EXPECT_EQ(model::AddressSet(addressesToUpdate.cbegin(), addressesToUpdate.cend()), view->addressesToUpdate());
		}
	}

	TEST(TEST_CLASS, CanLoadSummaryZeroHighValueAddresses) {
		// Assert:
		RunSummaryLoadTest(0);
	}

	TEST(TEST_CLASS, CanLoadSummarySingleHighValueAddress) {
		// Assert:
		RunSummaryLoadTest(1);
	}

	TEST(TEST_CLASS, CanLoadSummaryMultipleHighValueAddresses) {
		// Assert:
		RunSummaryLoadTest(3);
	}

	// endregion

	// region AccountStateCacheSubCachePlugin

	namespace {
		struct PluginTraits {
			static constexpr auto Base_Name = "AccountStateCache";

			class PluginType : public AccountStateCacheSubCachePlugin {
			public:
				explicit PluginType(const CacheConfiguration& config)
						: AccountStateCacheSubCachePlugin(config, CreateAccountStateCacheOptions(config::CreateMockConfigurationHolder()))
				{}
			};
		};
	}

	DEFINE_SUMMARY_AWARE_CACHE_STORAGE_PLUGIN_TESTS(PluginTraits)

	// endregion
}}
