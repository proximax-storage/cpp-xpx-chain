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

#include "catapult/chain/BlockDifficultyScorer.h"
#include "catapult/utils/Logging.h"
#include "tests/test/core/mocks/MockLocalNodeConfigurationHolder.h"
#include "tests/TestHarness.h"
#include "catapult/constants.h"
#include <cmath>

namespace catapult { namespace chain {

#define TEST_CLASS BlockDifficultyScorerTests

	namespace {
		using DifficultySet = cache::BlockDifficultyCacheTypes::PrimaryTypes::BaseSetType::SetType::MemorySetType;

		constexpr Difficulty Base_Difficulty = Difficulty(12345 * 3);

		cache::DifficultyInfoRange ToRange(const DifficultySet& set) {
			return cache::DifficultyInfoRange(set.cbegin(), set.cend());
		}

		model::BlockChainConfiguration CreateConfiguration() {
			auto config = model::BlockChainConfiguration::Uninitialized();
			config.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(60);
			config.MaxRollbackBlocks = 7;
			config.MaxDifficultyBlocks = 3;
			config.BlockTimeSmoothingFactor = 3000;
			return config;
		}
	}

	namespace {
		void AssertCalculatedDifficultyIsBaseDifficulty(const DifficultySet& set, Difficulty expectedDifficulty = Difficulty(0)) {
			// Act:
			auto difficulty = CalculateDifficulty(
					ToRange(set),
					state::BlockDifficultyInfo(Height(2), Timestamp(60010), Difficulty()),
					CreateConfiguration()
			);

			// Assert:
			EXPECT_EQ(expectedDifficulty, difficulty);
		}
	}

	TEST(TEST_CLASS, CalculatingDifficultyOnEmptyRangeYieldsBaseDifficulty) {
		// Arrange:
		DifficultySet set;

		// Assert:
		AssertCalculatedDifficultyIsBaseDifficulty(set);
	}

	TEST(TEST_CLASS, CalculatingDifficultyOnSingleSample) {
		// Arrange:
		DifficultySet set;
		set.emplace(Height(1), Timestamp(10), Difficulty(75'000));

		// Assert:
		AssertCalculatedDifficultyIsBaseDifficulty(set, Difficulty(75'000));
	}

	TEST(TEST_CLASS, CalculatingDifficultyOnSingleSampleOnWrongHeightYieldsBaseDifficulty) {
		// Arrange:
		DifficultySet set;
		set.emplace(Height(10), Timestamp(10), Difficulty(75'000'000'000'000));

		// Assert:
		EXPECT_THROW(AssertCalculatedDifficultyIsBaseDifficulty(set), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, CalculatingDifficultyOnSingleSampleWithZeroLastDifficulty) {
		// Arrange:
		DifficultySet set;
		set.emplace(Height(1), Timestamp(10), Difficulty(0));

		// Assert:
		EXPECT_THROW(AssertCalculatedDifficultyIsBaseDifficulty(set), catapult_invalid_argument);
	}

	namespace {
		Difficulty GetBlockDifficultyWithConstantTimeSpacing(uint32_t targetSpacing, uint32_t actualSpacing) {
			// Arrange:
			DifficultySet set;
			for (auto i = 0u; i < 4; ++i)
				set.emplace(Height(100 + i), Timestamp(12345 + i * actualSpacing), Base_Difficulty);

			auto nextBlockInfo = *(--set.end());
			set.erase(--set.end());

			auto config = CreateConfiguration();
			config.BlockGenerationTargetTime = utils::TimeSpan::FromMilliseconds(targetSpacing);

			// Act:
			return CalculateDifficulty(ToRange(set), nextBlockInfo, config);
		}
	}

	TEST(TEST_CLASS, BaseDifficultyIsNotChangedWhenBlocksHaveDesiredTargetTime) {
		// Arrange:
		auto difficulty = GetBlockDifficultyWithConstantTimeSpacing(75'000, 75'000);

		// Assert:
		EXPECT_EQ(Base_Difficulty, difficulty);
	}

	TEST(TEST_CLASS, BaseDifficultyIsIncreasedWhenBlocksHaveTimeLessThanTarget) {
		// Arrange:
		auto difficulty = GetBlockDifficultyWithConstantTimeSpacing(75'000, 76'000);

		// Assert:
		EXPECT_LT(Base_Difficulty, difficulty);
	}

	TEST(TEST_CLASS, BaseDifficultyIsDecreasedWhenBlocksHaveTimeHigherThanTarget) {
		// Arrange:
		auto difficulty = GetBlockDifficultyWithConstantTimeSpacing(75'000, 74'000);

		// Assert:
		EXPECT_GT(Base_Difficulty, difficulty);
	}

	namespace {
		void PrepareCache(cache::BlockDifficultyCache& cache, size_t numInfos) {
			auto delta = cache.createDelta(Height{0});
			for (auto i = 0u; i < numInfos; ++i)
				delta->insert(Height(i + 1), Timestamp(60'000 * i), Difficulty(1 + NEMESIS_BLOCK_DIFFICULTY * i));

			cache.commit();
		}

		struct CacheTraits {
			static Difficulty CalculateDifficulty(
					const cache::BlockDifficultyCache& cache,
					state::BlockDifficultyInfo nextBlockInfo,
					const model::BlockChainConfiguration& config) {
				return chain::CalculateDifficulty(cache, nextBlockInfo, config);
			}

			static void AssertDifficultyCalculationFailure(
					const cache::BlockDifficultyCache& cache,
					state::BlockDifficultyInfo nextBlockInfo,
					const model::BlockChainConfiguration& config) {
				// Act + Assert:
				EXPECT_THROW(chain::CalculateDifficulty(cache, nextBlockInfo, config), catapult_invalid_argument);
			}
		};

		struct TryCacheTraits {
			static Difficulty CalculateDifficulty(
					const cache::BlockDifficultyCache& cache,
					state::BlockDifficultyInfo nextBlockInfo,
					const model::BlockChainConfiguration& config) {
				Difficulty difficulty;
				EXPECT_TRUE(TryCalculateDifficulty(cache, nextBlockInfo, config, difficulty));
				return difficulty;
			}

			static void AssertDifficultyCalculationFailure(
					const cache::BlockDifficultyCache& cache,
					state::BlockDifficultyInfo nextBlockInfo,
					const model::BlockChainConfiguration& config) {
				Difficulty difficulty;
				EXPECT_FALSE(TryCalculateDifficulty(cache, nextBlockInfo, config, difficulty));
			}
		};
	}

#define CACHE_OVERLOAD_TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Cache) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<CacheTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_TryCache) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<TryCacheTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	CACHE_OVERLOAD_TRAITS_BASED_TEST(DifferentOverloadsYieldSameResult) {
		// Arrange:
		auto count = 10u;
		auto config = CreateConfiguration();
		cache::BlockDifficultyCache cache(config::CreateMockConfigurationHolder(config));
		PrepareCache(cache, count);
		state::BlockDifficultyInfo nextBlockInfo(
				Height(count + 1),
				Timestamp(60'000 * count),
				Difficulty()
		);

		// Act:
		auto difficulty1 = TTraits::CalculateDifficulty(cache, nextBlockInfo, config);

		auto view = cache.createView(Height{0});
		auto difficulty2 = CalculateDifficulty(view->difficultyInfos(Height(count), count), nextBlockInfo, config);

		// Assert:
		EXPECT_EQ(difficulty1, difficulty2);
	}

	CACHE_OVERLOAD_TRAITS_BASED_TEST(MaxDifficultyBlocksInConfigIsRespected) {
		// Arrange:
		auto count = 10u;
		auto config = CreateConfiguration();
		cache::BlockDifficultyCache cache(config::CreateMockConfigurationHolder(config));
		PrepareCache(cache, count);
		config.MaxDifficultyBlocks = 3;
		state::BlockDifficultyInfo nextBlockInfo(
				Height(count + 1),
				Timestamp(60'000 * count),
				Difficulty()
		);

		// Act:
		auto difficulty = TTraits::CalculateDifficulty(cache, nextBlockInfo, config);

		// Assert:
		EXPECT_EQ(Difficulty::Min() + Difficulty::Unclamped(9001), difficulty);
	}

	CACHE_OVERLOAD_TRAITS_BASED_TEST(CannotCalculateDifficultyIfStartingHeightIsNotInCache) {
		// Arrange:
		auto count = 10u;
		auto config = CreateConfiguration();
		cache::BlockDifficultyCache cache(config::CreateMockConfigurationHolder(config));
		PrepareCache(cache, count);
		state::BlockDifficultyInfo nextBlockInfo(
				Height(count + 2),
				Timestamp(60'000 * count),
				Difficulty()
		);

		// Act + Assert: try to calculate the difficulty for a height two past the last info
		TTraits::AssertDifficultyCalculationFailure(cache, nextBlockInfo, config);
	}
}}
