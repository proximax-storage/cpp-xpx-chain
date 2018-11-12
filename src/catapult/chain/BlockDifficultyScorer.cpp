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

#include "BlockDifficultyScorer.h"
#include <boost/multiprecision/cpp_int.hpp>

namespace catapult { namespace chain {

	namespace {
		constexpr uint64_t GAMMA_NUMERATOR{64};
		constexpr uint64_t GAMMA_DENOMINATOR{100};
		constexpr uint32_t SMOOTHING_FACTOR_DENOMINATOR{1000};
		constexpr uint64_t NEMESIS_BLOCK_DIFFICULTY{1000};

		constexpr utils::TimeSpan TimeDifference(const Timestamp& firstTimestamp, const Timestamp& lastTimestamp) {
			return utils::TimeSpan::FromDifference(lastTimestamp, firstTimestamp);
		}
	}

	Difficulty CalculateDifficulty(const cache::DifficultyInfoRange& difficultyInfos, const model::BlockChainConfiguration& config) {
		// note that difficultyInfos is sorted by both heights and timestamps, so the first info has the smallest
		// height and earliest timestamp and the last info has the largest height and latest timestamp
		size_t historySize = std::distance(difficultyInfos.begin(), difficultyInfos.end());

		if (historySize == 1 && difficultyInfos.begin()->BlockHeight == Height(1))
			return Difficulty(NEMESIS_BLOCK_DIFFICULTY);

		if (historySize < 2)
			return Difficulty(0);

		auto firstTimestamp = difficultyInfos.begin()->BlockTimestamp;

		const auto& lastInfo = *(--difficultyInfos.end());
		auto lastTimestamp = lastInfo.BlockTimestamp;
		auto timeDiff = TimeDifference(firstTimestamp, lastTimestamp);

		// Calculate the base target and return it as difficulty:
		// If S > 60
		//     Tb = (Tp * Min(S, MAXRATIO)) / 60
		// Else
		//     Tb = Tp - Tp * GAMMA * (60 - Max(S, MINRATIO)) / 60;
		// where:
		// S - average block time for the last 3 blocks
		// Tp - previous base target
		// Tb - calculated base target
		boost::multiprecision::uint128_t Tp = lastInfo.BlockDifficulty.unwrap();
		auto S = (timeDiff / (historySize - 1)).seconds();
		auto RATIO = config.BlockGenerationTargetTime.seconds();

		if (RATIO <= 0)
			CATAPULT_THROW_INVALID_ARGUMENT("BlockGenerationTargetTime is invalid or not set");

		auto factor = config.BlockTimeSmoothingFactor / SMOOTHING_FACTOR_DENOMINATOR;
		auto MINRATIO = RATIO - factor;
		auto MAXRATIO = RATIO + factor;
		Tp = (S > RATIO) ?
			Tp * std::min(S, MAXRATIO) / RATIO :
			Tp - Tp * GAMMA_NUMERATOR * (RATIO - std::max(S, MINRATIO) ) / GAMMA_DENOMINATOR / RATIO;

		return Difficulty(Tp.convert_to<uint64_t>());
	}

	namespace {
		Difficulty CalculateDifficulty(
				const cache::BlockDifficultyCacheView& view,
				Height height,
				const model::BlockChainConfiguration& config) {
			auto infos = view.difficultyInfos(height, config.MaxDifficultyBlocks);
			return chain::CalculateDifficulty(infos, config);
		}
	}

	Difficulty CalculateDifficulty(const cache::BlockDifficultyCache& cache, Height height, const model::BlockChainConfiguration& config) {
		auto view = cache.createView();
		return CalculateDifficulty(*view, height, config);
	}

	bool TryCalculateDifficulty(
			const cache::BlockDifficultyCache& cache,
			Height height,
			const model::BlockChainConfiguration& config,
			Difficulty& difficulty) {
		auto view = cache.createView();
		if (!view->contains(state::BlockDifficultyInfo(height)))
			return false;

		difficulty = CalculateDifficulty(*view, height, config);
		return true;
	}
}}
