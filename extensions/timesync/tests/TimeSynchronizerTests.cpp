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

#include "timesync/src/TimeSynchronizer.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/model/Address.h"
#include "timesync/tests/test/TimeSynchronizationTestUtils.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/local/ServiceLocatorTestContext.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"

namespace catapult { namespace timesync {

#define TEST_CLASS TimeSynchronizerTests

	namespace {
		constexpr int64_t Warning_Threshold_Millis = 5'000;
		constexpr auto Total_Chain_Importance = Importance(1'000'000'000);
		constexpr model::NetworkIdentifier Default_Network_Identifier = model::NetworkIdentifier::Mijin_Test;
		constexpr auto Harvesting_Mosaic_Id = MosaicId(9876);

		filters::SynchronizationFilter CreateSynchronizationFilter(size_t& numFilterCalls) {
			return [&numFilterCalls](const auto&, auto) {
				++numFilterCalls;
				return false;
			};
		}

		template<typename TKey>
		void AddAccount(
				cache::AccountStateCache& cache,
				const TKey& key,
				Importance importance) {
			auto delta = cache.createDelta(Height{0});
			delta->addAccount(key, Height(1));
			delta->find(key).get().Balances.credit(Harvesting_Mosaic_Id, Amount(importance.unwrap()), Height(1));
			cache.commit();
		}

		template<typename TKey>
		void SeedAccountStateCache(
				cache::AccountStateCache& cache,
				const std::vector<TKey>& keys,
				const std::vector<Importance>& importances) {
			for (auto i = 0u; i < keys.size(); ++i)
				AddAccount(cache, keys[i], importances[i]);
		}

		std::vector<Address> ToAddresses(const std::vector<Key>& keys) {
			std::vector<Address> addresses;
			for (const auto& key : keys)
				addresses.push_back(model::PublicKeyToAddress(key, Default_Network_Identifier));

			return addresses;
		}

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.NetworkIdentifier = model::NetworkIdentifier::Mijin_Test;
			config.Immutable.CurrencyMosaicId = MosaicId(1111);
			config.Immutable.HarvestingMosaicId = Harvesting_Mosaic_Id;
			config.Network.ImportanceGrouping = 234;
			config.Network.MinHarvesterBalance = Amount(1000);
			config.Network.TotalChainImportance = Total_Chain_Importance;
			return config.ToConst();
		}

		enum class KeyType { Address, PublicKey, };

		class TestContext {
		public:
			explicit TestContext(
					const std::vector<std::pair<int64_t, uint64_t>>& offsetsAndRawImportances,
					const std::vector<filters::SynchronizationFilter>& filters = {},
					KeyType keyType = KeyType::PublicKey)
					: m_configHolder(config::CreateMockConfigurationHolder(CreateConfig()))
					, m_state(test::CreateCatapultCacheWithMarkerAccount(m_configHolder->Config(Height(0))), m_configHolder)
					, m_cache(const_cast<cache::AccountStateCache&>(m_state.cache().sub<cache::AccountStateCache>()))
					, m_synchronizer(filters::AggregateSynchronizationFilter(filters), m_state.state(), Warning_Threshold_Millis) {
				std::vector<Importance> importances;
				for (const auto& offsetAndRawImportance : offsetsAndRawImportances) {
					m_samples.emplace(test::CreateTimeSyncSampleWithTimeOffset(offsetAndRawImportance.first));
					importances.push_back(Importance(offsetAndRawImportance.second));
				}

				auto keys = test::ExtractKeys(m_samples);
				auto addresses = ToAddresses(keys);
				if (KeyType::PublicKey == keyType)
					SeedAccountStateCache(m_cache, keys, importances);
				else
					SeedAccountStateCache(m_cache, addresses, importances);
			}

		public:
			TimeOffset calculateTimeOffset(NodeAge nodeAge = NodeAge()) {
				return m_synchronizer.calculateTimeOffset(*m_cache.createView(Height{0}), Height(1), std::move(m_samples), nodeAge);
			}

			void addHighValueAccounts(size_t count) {
				for (auto i = 0u; i < count; ++i)
					AddAccount(m_cache, test::GenerateRandomByteArray<Key>(), Importance(100'000));
			}

		private:
			std::shared_ptr<config::BlockchainConfigurationHolder> m_configHolder;
			test::ServiceTestState m_state;
			cache::AccountStateCache& m_cache;
			TimeSynchronizer m_synchronizer;
			TimeSynchronizationSamples m_samples;
		};
	}

	// region basic tests

	TEST(TEST_CLASS, TimeSynchronizerDelegatesToFilter) {
		// Arrange:
		size_t numFilterCalls = 0;
		auto filter = CreateSynchronizationFilter(numFilterCalls);
		TestContext context({ { 12, 0 }, { 23, 0 }, { 34, 0 } }, { filter });

		// Act:
		context.calculateTimeOffset();

		// Assert: 3 samples were passed to the filter
		EXPECT_EQ(3u, numFilterCalls);
	}

	TEST(TEST_CLASS, TimeSynchronizerReturnsZeroTimeOffsetWhenNoSamplesAreAvailable) {
		// Arrange:
		TestContext context({}, {});

		// Act:
		auto timeOffset = context.calculateTimeOffset();

		// Assert:
		EXPECT_EQ(TimeOffset(), timeOffset);
	}

	TEST(TEST_CLASS, TimeSynchronizerReturnsZeroTimeOffsetWhenCumulativeImportanceIsZero) {
		// Arrange:
		TestContext context({ { 12, 0 }, { 23, 0 } });

		// Act:
		auto timeOffset = context.calculateTimeOffset();

		// Assert:
		EXPECT_EQ(TimeOffset(), timeOffset);
	}

	TEST(TEST_CLASS, TimeSynchronizerSucceedsIfOnlyAddressIsKnownToAccountStateCache) {
		// Arrange: seed account state cache only with addresses
		TestContext context({ { 100, 500'000'000 }, { 200, 500'000'000 } }, {}, KeyType::Address);

		// Act: should not throw
		auto timeOffset = context.calculateTimeOffset();

		// Assert:
		EXPECT_EQ(TimeOffset(150), timeOffset);
	}

	// endregion

	// region coupling

	namespace {
		void AssertCorrectTimeOffsetWithCoupling(NodeAge nodeAge, TimeOffset expectedTimeOffset) {
			// Arrange:
			auto numSamples = 100u;
			filters::AggregateSynchronizationFilter aggregateFilter({});
			auto samples = test::CreateTimeSyncSamplesWithIncreasingTimeOffset(1000, numSamples);
			auto keys = test::ExtractKeys(samples);
			auto configHolder = config::CreateMockConfigurationHolder(CreateConfig());
			test::ServiceTestState state(test::CreateCatapultCacheWithMarkerAccount(configHolder->Config(Height(0))), configHolder);
			auto& cache = const_cast<cache::AccountStateCache&>(state.cache().sub<cache::AccountStateCache>());
			auto singleAccountImportance = Importance(Total_Chain_Importance.unwrap() / numSamples);
			SeedAccountStateCache(cache, keys, std::vector<Importance>(numSamples, singleAccountImportance));
			TimeSynchronizer synchronizer(aggregateFilter, state.state(), Warning_Threshold_Millis);

			// Act:
			auto timeOffset = synchronizer.calculateTimeOffset(*cache.createView(Height{0}), Height(1), std::move(samples), nodeAge);

			// Assert:
			EXPECT_EQ(expectedTimeOffset, timeOffset);
		}
	}

	TEST(TEST_CLASS, TimeSynchronizerReturnsCorrectTimeOffset_MaximumCoupling) {
		// Arrange: 100 samples with offsets 1000, 1001, ..., 1099. All accounts have the same importance.
		// - the expected offset is therefore the mean value multiplied with the coupling
		auto rawExpectedOffset = static_cast<int64_t>((100'000 + 99 * 100 / 2) * Coupling_Start / 100);

		// Assert:
		AssertCorrectTimeOffsetWithCoupling(NodeAge(), TimeOffset(rawExpectedOffset));
	}

	TEST(TEST_CLASS, TimeSynchronizerReturnsCorrectTimeOffset_IntermediateCoupling) {
		// Arrange: 100 samples with offsets 1000, 1001, ..., 1099. All accounts have the same importance.
		// - the expected offset is therefore the mean value multiplied with the coupling
		constexpr uint64_t ageOffset = 5;
		auto coupling = std::exp(-Coupling_Decay_Strength * ageOffset);
		auto rawExpectedOffset = static_cast<int64_t>((100'000 + 99 * 100 / 2) * coupling / 100);

		// Assert:
		AssertCorrectTimeOffsetWithCoupling(NodeAge(Start_Coupling_Decay_After_Round + ageOffset), TimeOffset(rawExpectedOffset));
	}

	TEST(TEST_CLASS, TimeSynchronizerReturnsCorrectTimeOffset_MinimumCoupling) {
		// Arrange: 100 samples with offsets 1000, 1001, ..., 1099. All accounts have the same importance.
		// - the expected offset is therefore the mean value multiplied with the coupling
		auto rawExpectedOffset = static_cast<int64_t>((100'000 + 99 * 100 / 2) * Coupling_Minimum / 100);

		// Assert:
		AssertCorrectTimeOffsetWithCoupling(NodeAge(Start_Coupling_Decay_After_Round + 10), TimeOffset(rawExpectedOffset));
	}

	// endregion

	// region importance dependency

	TEST(TEST_CLASS, TimeSynchronizerReturnsCorrectTimeOffset_ImportanceDependency) {
		// Arrange: scaling is 1 in order to solely test the influence of importance
		TestContext context({ { 100, 100'000'000 }, { 500, 900'000'000 } });

		// Act:
		auto timeOffset = context.calculateTimeOffset();

		// Assert: 1 / 10 * 100 + 9 / 10 * 500 = 460
		EXPECT_EQ(TimeOffset(460), timeOffset);
	}

	// endregion

	// region scaling

	TEST(TEST_CLASS, TimeSynchronizerScaling_ViewPercentageDominant_Max) {
		// Arrange: importance percentage = 1 / 10, view percentage = 1: scaling == 1
		TestContext context({ { 100, 50'000'000 }, { 100, 50'000'000 } });

		// Act:
		auto timeOffset = context.calculateTimeOffset();

		// Assert: 1 / 10 * 100
		EXPECT_EQ(TimeOffset(10), timeOffset);
	}

	TEST(TEST_CLASS, TimeSynchronizerScaling_ViewPercentageDominant_Half) {
		// Arrange: importance percentage = 1 / 10, view percentage = 1 / 2: scaling == 2
		TestContext context({ { 100, 50'000'000 }, { 100, 50'000'000 } });

		// - add another 2 high value accounts
		context.addHighValueAccounts(2);

		// Act:
		auto timeOffset = context.calculateTimeOffset();

		// Assert: 2 * 1 / 10 * 100
		EXPECT_EQ(TimeOffset(2 * 10), timeOffset);
	}

	TEST(TEST_CLASS, TimeSynchronizerScaling_ImportancePercentageDominant_Max) {
		// Arrange: importance percentage = 1, view percentage = 1 / 5: scaling == 1
		TestContext context({ { 100, 500'000'000 }, { 100, 500'000'000 } });

		// - add another 8 high value accounts
		context.addHighValueAccounts(8);

		// Act:
		auto timeOffset = context.calculateTimeOffset();

		// Assert: 1 * 100
		EXPECT_EQ(TimeOffset(100), timeOffset);
	}

	TEST(TEST_CLASS, TimeSynchronizerScaling_ImportancePercentageDominant_Half) {
		// Arrange: importance percentage = 1 / 2, view percentage = 1 / 5: scaling == 2
		TestContext context({ { 100, 250'000'000 }, { 100, 250'000'000 } });

		// - add another 8 high value accounts
		context.addHighValueAccounts(8);

		// Act:
		auto timeOffset = context.calculateTimeOffset();

		// Assert: 2 * 1 / 2 * 100
		EXPECT_EQ(TimeOffset(100), timeOffset);
	}

	// endregion
}}
