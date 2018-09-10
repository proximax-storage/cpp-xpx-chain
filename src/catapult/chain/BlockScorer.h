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
#include "catapult/model/BlockChainConfiguration.h"
#include "catapult/types.h"
#include <functional>

namespace catapult { namespace model { struct Block; } }

namespace catapult { namespace chain {

	/// Calculates the hit for a \a generationHash.
	uint64_t CalculateHit(const Hash256& generationHash);

	/// Calculates the score of \a currentBlock with parent \a parentBlock.
	uint64_t CalculateScore(const model::Block& parentBlock, const model::Block& currentBlock);

	/// Contextual information for calculating a block hit.
	struct BlockHitContext {
	public:
		/// Creates a block hit context.
		BlockHitContext() : ElapsedTime(utils::TimeSpan::FromSeconds(0))
		{}

	public:
		/// Generation hash.
		Hash256 GenerationHash;

		/// Time since the last block.
		utils::TimeSpan ElapsedTime;

		/// Public key of the block signer.
		Key Signer;

		/// Block difficulty.
		catapult::Difficulty Difficulty;

		/// Block height.
		catapult::Height Height;

		const model::Block parentBlock;

		const model::Block currentBlock;
	};

	/// Predicate used to determine if a block is a hit or not.
	class BlockHitPredicate {
	private:
		using ImportanceLookupFunc = std::function<Importance (const Key&, Height)>;

	public:
		/// Creates a predicate around a block chain configuration (\a config) and an importance lookup function
		/// (\a importanceLookup).
		BlockHitPredicate(const model::BlockChainConfiguration& config, const ImportanceLookupFunc& importanceLookup);

	public:
		/// Determines if the \a block is a hit given its parent (\a parentBlock) and generation hash (\a generationHash).
		bool operator()(const model::Block& parentBlock, const model::Block& block, const Hash256& generationHash) const;

		/// Determines if the specified \a context is a hit.
		bool operator()(const BlockHitContext& context) const;

	private:
		model::BlockChainConfiguration m_config;
		ImportanceLookupFunc m_importanceLookup;
	};
}}
