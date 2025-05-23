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
#include "catapult/model/NetworkConfiguration.h"
#include "catapult/types.h"
#include <boost/multiprecision/cpp_int.hpp>
#include <functional>

namespace catapult {
	namespace config { struct BlockchainConfigurationHolder; }
	namespace model { struct Block; }
}

namespace catapult { namespace chain {

	using BlockTarget = boost::multiprecision::uint256_t;

	/// Calculates the hit for a \a generationHash.
	uint64_t CalculateHit(const GenerationHash& generationHash);

	/// Calculates the score of \a currentBlock with parent \a parentBlock.
	uint64_t CalculateScore(const model::Block& parentBlock, const model::Block& currentBlock);

	/// Calculates the target from a time span (\a timeSpan), a \a difficulty and an effective signer importance
	/// of \a signerImportance for the block chain described by \a config.
	BlockTarget CalculateTarget(
			const utils::TimeSpan& timeSpan,
			const Difficulty& difficulty,
			const Importance& signerImportance,
			const model::NetworkConfiguration& config,
			uint32_t feeInterest,
			uint32_t feeInterestDenominator);

	/// Calculates the target of \a currentBlock with parent \a parentBlock and effective signer importance
	/// of \a signerImportance for the block chain described by \a config.
	BlockTarget CalculateTarget(
			const model::Block& parentBlock,
			const model::Block& currentBlock,
			const Importance& signerImportance,
			const model::NetworkConfiguration& config);

	/// Contextual information for calculating a block hit.
	struct BlockHitContext {
	public:
		/// Creates a block hit context.
		BlockHitContext() : ElapsedTime(utils::TimeSpan::FromSeconds(0))
		{}

	public:
		/// Generation hash.
		catapult::GenerationHash GenerationHash;

		/// Time since the last block.
		utils::TimeSpan ElapsedTime;

		/// Public key of the block signer.
		Key Signer;

		/// Block difficulty.
		catapult::Difficulty Difficulty;

		/// Block height.
		catapult::Height Height;

		/// The part of the transaction fee harvester is willing to get.
		/// From 0 up to FeeInterestDenominator. The customer gets
		/// (FeeInterest / FeeInterestDenominator)'th part of the maximum transaction fee.
		uint32_t FeeInterest;

		/// Denominator of the transaction fee.
		uint32_t FeeInterestDenominator;
	};

	/// Predicate used to determine if a block is a hit or not.
	class BlockHitPredicate {
	private:
		using ImportanceLookupFunc = std::function<Importance (const Key&, Height)>;

	public:
		/// Creates a predicate around a block chain configuration (\a config) and an importance lookup function
		/// (\a importanceLookup).
		BlockHitPredicate(std::shared_ptr<config::BlockchainConfigurationHolder>  pConfigHolder, ImportanceLookupFunc  importanceLookup);

	public:
		/// Determines if the \a block is a hit given its parent (\a parentBlock) and generation hash (\a generationHash).
		bool operator()(const model::Block& parentBlock, const model::Block& block, const GenerationHash& generationHash) const;

		/// Determines if the specified \a context is a hit.
		bool operator()(const BlockHitContext& context) const;

	private:
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
		ImportanceLookupFunc m_importanceLookup;
	};
}}
