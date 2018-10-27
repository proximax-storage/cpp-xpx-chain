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

#pragma once
#include "catapult/cache_core/ReadOnlyAccountStateCache.h"
#include "catapult/constants.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "catapult/state/AccountState.h"
#include "catapult/types.h"
#include <functional>

namespace catapult { namespace model { struct BlockHitContext; } }

namespace catapult { namespace chain {

	// The first 8 bytes of a \a generationHash are converted to a number, referred to as the account hit.
	uint64_t CalculateHit(const Hash256& generationHash);

	// Each account calculates its own target value, based on its current effective stake. This value is:
	// T = Tb x S x Be
	// where:
	// T is the new target value
	// Tb is the base target value
	// S is the time since the last block, in seconds
	// Be is the effective balance of the account
	Target CalculateTarget(
			const BlockTarget& baseTarget,
			const utils::TimeSpan& elapsedTime,
			const Amount& effectiveBalance);

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
		const utils::TimeSpan& averageBlockTime,
		const model::BlockChainConfiguration& config);

	/// Predicate used to determine if a block is a hit or not.
	class BlockHitPredicate {
	public:
		/// Determines if the \a block is a hit given generation hash (\a generationHash) and time elapsed since last block (\a ElapsedTime).
		bool operator()(const Hash256& generationHash, const BlockTarget& parentBaseTarget,
				const utils::TimeSpan& elapsedTime, const Amount& effectiveBalance) const;

		/// Determines if the specified \a context is a hit.
		bool operator()(const model::BlockHitContext& context) const;
	};
}}
