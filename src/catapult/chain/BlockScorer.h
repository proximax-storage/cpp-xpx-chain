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

	BlockTarget CalculateBaseTarget(
		const BlockTarget& parentBaseTarget,
		const utils::TimeSpan& averageBlockTime,
		const model::BlockChainConfiguration& config);

	// Calculate effective balance of account = min(Balance[currentHeight], Balance[previousHeight])
	Amount CalculateEffectiveBalance(
			const cache::ReadOnlyAccountStateCache& currentCache,
			const cache::ReadOnlyAccountStateCache& previousCache,
			const Height& effectiveBalanceHeight,
			const Height& currentHeight,
			const Key& signer);

	/// Predicate used to determine if a block is a hit or not.
	class BlockHitPredicate {
	public:
		/// Determines if the \a block is a hit given generation hash (\a generationHash) and time elapsed since last block (\a ElapsedTime).
		bool operator()(const Hash256& generationHash, const BlockTarget& parentBaseTarget,
				const utils::TimeSpan& ElapsedTime, const Amount& effectiveBalance) const;

		/// Determines if the specified \a context is a hit.
		bool operator()(const model::BlockHitContext& context) const;
	};
}}
