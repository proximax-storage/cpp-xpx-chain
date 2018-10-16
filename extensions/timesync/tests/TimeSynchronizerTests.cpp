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

#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/model/Address.h"
#include "tests/catapult/extensions/test/LocalNodeStateUtils.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/TestHarness.h"
#include "timesync/src/constants.h"
#include "timesync/src/TimeSynchronizer.h"
#include "timesync/tests/test/TimeSynchronizationTestUtils.h"
#include <cmath>

namespace catapult { namespace timesync {

#define TEST_CLASS TimeSynchronizerTests

	namespace {
		constexpr int64_t Warning_Threshold_Millis = 5'000;
		constexpr uint64_t Total_Chain_Balance = 1'000'000'000;
		constexpr model::NetworkIdentifier Default_Network_Identifier = model::NetworkIdentifier::Mijin_Test;

		filters::SynchronizationFilter CreateSynchronizationFilter(size_t& numFilterCalls) {
			return [&numFilterCalls](const auto&, auto) {
				++numFilterCalls;
				return false;
			};
		}

		template<typename TKey>
		void AddAccount(
				cache::CatapultCache& cache,
				const TKey& key,
				uint64_t balance) {
			auto delta = cache.createDelta();
			auto& accountCache = delta.sub<cache::AccountStateCache>();
			auto& accountState = accountCache.addAccount(key, Height(100));
			accountState.Balances.credit(Xpx_Id, Amount(balance), Height(100));
			cache.commit(Height(100));
		}

		template<typename TKey>
		void AddAccount(
				cache::CatapultCache& cache,
				const TKey& key) {
			AddAccount(cache, key, 1000);
		}

		template<typename TKey>
		void SeedAccountStateCache(
				cache::CatapultCache& cache,
				const std::vector<TKey>& keys,
				const std::vector<int64_t>& balances) {
			for (auto i = 0u; i < keys.size(); ++i)
				AddAccount(cache, keys[i], balances[i]);
		}

		std::vector<Address> ToAddresses(const std::vector<Key>& keys) {
			std::vector<Address> addresses;
			for (const auto& key : keys)
				addresses.push_back(model::PublicKeyToAddress(key, Default_Network_Identifier));

			return addresses;
		}

		enum class KeyType { Address, PublicKey, };

		model::BlockChainConfiguration createConfig() {
			auto blockConfig = model::BlockChainConfiguration::Uninitialized();
			blockConfig.MinHarvesterBalance = Amount(1000);
			blockConfig.Network.Identifier = Default_Network_Identifier;

			return blockConfig;
		}

		class TestContext {
		public:
			explicit TestContext(
					const std::vector<std::pair<int64_t, int64_t>>& offsetsAndBalances,
					const std::vector<filters::SynchronizationFilter>& filters = {},
					KeyType keyType = KeyType::PublicKey)
					: m_cache({})
					, m_synchronizer(filters::AggregateSynchronizationFilter(filters), Total_Chain_Balance, Warning_Threshold_Millis) {
				std::vector<int64_t> balances;
				for (const auto& offsetsAndBalance : offsetsAndBalances) {
					m_samples.emplace(test::CreateTimeSyncSampleWithTimeOffset(offsetsAndBalance.first));
					balances.push_back(offsetsAndBalance.second);
				}

				m_cache = createCache(keyType, balances);
			}

		public:
			TimeOffset calculateTimeOffset(NodeAge nodeAge = NodeAge()) {
				auto state = extensions::LocalNodeStateRef(
						*test::LocalNodeStateUtils::CreateLocalNodeState(),
						m_cache
				);
				return m_synchronizer.calculateTimeOffset(state, Height(1), std::move(m_samples), nodeAge);
			}

			void addHighValueAccounts(size_t count) {
				for (auto i = 0u; i < count; ++i) {
					auto key = test::GenerateRandomData<Key_Size>();
					AddAccount(m_cache, key);
				}
			}

		private:
			cache::CatapultCache m_cache;
			TimeSynchronizer m_synchronizer;
			TimeSynchronizationSamples m_samples;

		private:
			cache::CatapultCache createCache(KeyType keyType, const std::vector<int64_t>& balances) {
				auto cache = test::CreateEmptyCatapultCache(createConfig());
				auto keys = test::ExtractKeys(m_samples);
				auto addresses = ToAddresses(keys);
				if (KeyType::PublicKey == keyType)
					SeedAccountStateCache(cache, keys, balances);
				else
					SeedAccountStateCache(cache, addresses, balances);

				return cache;
			}
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

	TEST(TEST_CLASS, TimeSynchronizerReturnsZeroTimeOffsetWhenCumulativeBalanceIsZero) {
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
			std::vector<int64_t> balances(numSamples, Total_Chain_Balance / numSamples);
			cache::CatapultCache cache = test::CreateEmptyCatapultCache(createConfig());
			SeedAccountStateCache(cache, keys, balances);
			TimeSynchronizer synchronizer(aggregateFilter, Total_Chain_Balance, Warning_Threshold_Millis);

			auto state = extensions::LocalNodeStateRef(*test::LocalNodeStateUtils::CreateLocalNodeState(), cache);
			// Act:
			auto timeOffset = synchronizer.calculateTimeOffset(state, Height(1), std::move(samples), nodeAge);

			// Assert:
			EXPECT_EQ(expectedTimeOffset, timeOffset);
		}
	}

	TEST(TEST_CLASS, TimeSynchronizerReturnsCorrectTimeOffset_MaximumCoupling) {
		// Arrange: 100 samples with offsets 1000, 1001, ..., 1099. All accounts have the same balance.
		// - the expected offset is therefore the mean value multiplied with the coupling
		auto rawExpectedOffset = static_cast<int64_t>((100'000 + 99 * 100 / 2) * Coupling_Start / 100);

		// Assert:
		AssertCorrectTimeOffsetWithCoupling(NodeAge(), TimeOffset(rawExpectedOffset));
	}

	TEST(TEST_CLASS, TimeSynchronizerReturnsCorrectTimeOffset_IntermediateCoupling) {
		// Arrange: 100 samples with offsets 1000, 1001, ..., 1099. All accounts have the same balance.
		// - the expected offset is therefore the mean value multiplied with the coupling
		constexpr uint64_t ageOffset = 5;
		auto coupling = std::exp(-Coupling_Decay_Strength * ageOffset);
		auto rawExpectedOffset = static_cast<int64_t>((100'000 + 99 * 100 / 2) * coupling / 100);

		// Assert:
		AssertCorrectTimeOffsetWithCoupling(NodeAge(Start_Coupling_Decay_After_Round + ageOffset), TimeOffset(rawExpectedOffset));
	}

	TEST(TEST_CLASS, TimeSynchronizerReturnsCorrectTimeOffset_MinimumCoupling) {
		// Arrange: 100 samples with offsets 1000, 1001, ..., 1099. All accounts have the same balance.
		// - the expected offset is therefore the mean value multiplied with the coupling
		auto rawExpectedOffset = static_cast<int64_t>((100'000 + 99 * 100 / 2) * Coupling_Minimum / 100);

		// Assert:
		AssertCorrectTimeOffsetWithCoupling(NodeAge(Start_Coupling_Decay_After_Round + 10), TimeOffset(rawExpectedOffset));
	}

	// endregion

	// region balance dependency

	TEST(TEST_CLASS, TimeSynchronizerReturnsCorrectTimeOffset_BalanceDependency) {
		// Arrange: scaling is 1 in order to solely test the influence of balance
		TestContext context({ { 100, 100'000'000 }, { 500, 900'000'000 } });

		// Act:
		auto timeOffset = context.calculateTimeOffset();

		// Assert: 1 / 10 * 100 + 9 / 10 * 500 = 460
		EXPECT_EQ(TimeOffset(460), timeOffset);
	}

	// endregion

	// region scaling

	TEST(TEST_CLASS, TimeSynchronizerScaling_ViewPercentageDominant_Max) {
		// Arrange: balance percentage = 1 / 10, view percentage = 1: scaling == 1
		TestContext context({ { 100, 50'000'000 }, { 100, 50'000'000 } });

		// Act:
		auto timeOffset = context.calculateTimeOffset();

		// Assert: 1 / 10 * 100
		EXPECT_EQ(TimeOffset(10), timeOffset);
	}

	TEST(TEST_CLASS, TimeSynchronizerScaling_ViewPercentageDominant_Half) {
		// Arrange: balance percentage = 1 / 10, view percentage = 1 / 2: scaling == 2
		TestContext context({ { 100, 50'000'000 }, { 100, 50'000'000 } });

		// - add another 2 high value accounts
		context.addHighValueAccounts(2);

		// Act:
		auto timeOffset = context.calculateTimeOffset();

		// Assert: 2 * 1 / 10 * 100
		EXPECT_EQ(TimeOffset(2 * 10), timeOffset);
	}

	TEST(TEST_CLASS, TimeSynchronizerScaling_BalancePercentageDominant_Max) {
		// Arrange: balance percentage = 1, view percentage = 1 / 5: scaling == 1
		TestContext context({ { 100, 500'000'000 }, { 100, 500'000'000 } });

		// - add another 8 high value accounts
		context.addHighValueAccounts(8);

		// Act:
		auto timeOffset = context.calculateTimeOffset();

		// Assert: 1 * 100
		EXPECT_EQ(TimeOffset(100), timeOffset);
	}

	TEST(TEST_CLASS, TimeSynchronizerScaling_BalancePercentageDominant_Half) {
		// Arrange: balance percentage = 1 / 2, view percentage = 1 / 5: scaling == 2
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
