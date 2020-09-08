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

#include "BlockScorer.h"
#include "catapult/model/Block.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"

namespace catapult { namespace chain {

	namespace {
		constexpr utils::TimeSpan TimeBetweenBlocks(const model::Block& parent, const model::Block& block) {
			return utils::TimeSpan::FromDifference(block.Timestamp, parent.Timestamp);
		}
	}

	uint64_t CalculateHit(const GenerationHash& generationHash) {
		return *reinterpret_cast<const uint64_t*>(generationHash.data());
	}

	uint64_t CalculateScore(const model::Block& parentBlock, const model::Block& currentBlock) {
		if (currentBlock.Timestamp <= parentBlock.Timestamp)
			return 0u;

		if (currentBlock.Difficulty.unwrap() == 0)
			CATAPULT_THROW_INVALID_ARGUMENT("Difficulty of block can't be zero");

		BlockTarget score{1u};
		score <<= 64u;
		score /= currentBlock.Difficulty.unwrap();
		return score.convert_to<uint64_t>();
	}

	BlockTarget CalculateTarget(
			const utils::TimeSpan& timeSpan,
			Difficulty difficulty,
			Importance signerImportance,
			const model::NetworkConfiguration& config,
			uint32_t feeInterest,
			uint32_t feeInterestDenominator) {
		double target(difficulty.unwrap());
		target *= timeSpan.seconds();
		target *= signerImportance.unwrap();
		double greed = feeInterest;
		greed /= feeInterestDenominator;
		double lambda = std::pow(1.0 + config.GreedDelta * (1.0 - 2.0 * greed), config.GreedExponent);
		if (lambda < 0.0)
			CATAPULT_THROW_INVALID_ARGUMENT("Target is negative");
		target *= lambda;
		return BlockTarget{target};
	}

	BlockTarget CalculateTarget(
			const model::Block& parentBlock,
			const model::Block& currentBlock,
			Importance signerImportance,
			const model::NetworkConfiguration& config) {
		if (currentBlock.Timestamp <= parentBlock.Timestamp)
			return BlockTarget(0);

		auto timeDiff = TimeBetweenBlocks(parentBlock, currentBlock);
		return CalculateTarget(timeDiff, currentBlock.Difficulty, signerImportance, config, currentBlock.FeeInterest, currentBlock.FeeInterestDenominator);
	}

	BlockHitPredicate::BlockHitPredicate(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder, const ImportanceLookupFunc& importanceLookup)
			: m_pConfigHolder(pConfigHolder)
			, m_importanceLookup(importanceLookup)
	{}

	bool BlockHitPredicate::operator()(const model::Block& parentBlock, const model::Block& block, const GenerationHash& generationHash) const {
		auto importance = m_importanceLookup(block.Signer, block.Height);
		auto hit = CalculateHit(generationHash);
		auto target = CalculateTarget(parentBlock, block, importance, m_pConfigHolder->Config(block.Height).Network);
		return hit < target;
	}

	bool BlockHitPredicate::operator()(const BlockHitContext& context) const {
		auto importance = m_importanceLookup(context.Signer, context.Height);
		auto hit = CalculateHit(context.GenerationHash);
		auto target = CalculateTarget(
			context.ElapsedTime,
			context.Difficulty,
			importance,
			m_pConfigHolder->Config(context.Height).Network,
			context.FeeInterest,
			context.FeeInterestDenominator);
		return hit < target;
	}
}}
