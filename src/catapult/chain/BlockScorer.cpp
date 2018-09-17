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
#include "catapult/state/AccountState.h"
#include "catapult/utils/IntegerMath.h"
#include "catapult/utils/TimeSpan.h"

namespace catapult { namespace chain {

	namespace {
		constexpr uint64_t MAXRATIO{67};
		constexpr uint64_t MINRATIO{53};
		constexpr double GAMMA{0.64};
		constexpr int BLOCK_GENERATION_TIME{0};

		/// The first 8 bytes of a \a generationHash are converted to a number, referred to as the account hit.
		uint64_t CalculateHit(const Hash256& generationHash) {
			return *reinterpret_cast<uint64_t*>(generationHash.data());
		}

		// Each account calculates its own target value, based on its current effective stake. This value is:
		// T = Tb x S x Be
		// where:
		// T is the new target value
		// Tb is the base target value
		// S is the time since the last block, in seconds
		// Be is the effective balance of the account
		BlockTarget CalculateTarget(
				const BlockTarget& baseTarget,
				const utils::TimeSpan& ElapsedTime,
				const Amount& effectiveBalance) {
			return baseTarget * ElapsedTime.seconds() * effectiveBalance;
		}
	}

	// The base target is calculated as follows:
	// If S>60
	//     Tb = (Tp * Min(S, MAXRATIO)) / 60
	// Else
	//     Tb = Tp - Tp * GAMMA * (60 - Max(S, MINRATIO)) / 60;
	// where:
	// S - average block time for the last 3 blocks
	// Tp - previous base target
	// Tb - calculated base target
	BlockTarget CalculateBaseTarget(
			const BlockTarget& parentBaseTarget,
			const utils::TimeSpan& averageBlockTime) {
		if (averageBlockTime > utils::TimeSpan::FromSeconds(BLOCK_GENERATION_TIME)) {
			return (parentBaseTarget * std::min(averageBlockTime.seconds(), MAXRATIO)) / BLOCK_GENERATION_TIME;
		} else {
			return (parentBaseTarget -
					parentBaseTarget * GAMMA * (BLOCK_GENERATION_TIME - std::max(averageBlockTime.seconds(), MINRATIO))) / BLOCK_GENERATION_TIME;
		}
	}

	bool BlockHitPredicate::operator()(const Hash256& generationHash, const BlockTarget& baseTarget,
			const utils::TimeSpan& ElapsedTime, const Amount& effectiveBalance) const {
		auto hit = CalculateHit(generationHash);
		auto target = CalculateTarget(baseTarget, ElapsedTime, effectiveBalance);
		return hit < target;
	}

	bool BlockHitPredicate::operator()(const BlockHitContext& context) const {
		auto hit = CalculateHit(context.GenerationHash);
		auto target = CalculateTarget(context.BaseTarget, context.ElapsedTime, context.EffectiveBalance);
		return hit < target;
	}
}}
