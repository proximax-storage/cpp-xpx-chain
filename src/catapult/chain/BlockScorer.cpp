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
		constexpr uint64_t Two_To_54{1ull << 54};

		constexpr uint64_t MAXRATIO{67};
		constexpr uint64_t MINRATIO{53};
		constexpr double GAMMA{0.64};
		constexpr int BLOCK_GENERATION_TIME{0};
		constexpr uint64_t TWO_TO_64{1ull << 64};

		struct GenerationHashInfo {
			uint32_t Value;
			uint32_t NumLeadingZeros;
		};

		uint32_t NumLeadingZeros(const Hash256& generationHash) {
			for (auto i = 0u; i < Hash256_Size; ++i) {
				if (0 != generationHash[i])
					return 8u * i + 7u - utils::Log2(generationHash[i]);
			}

			return Hash256_Size;
		}

#ifdef _MSC_VER
#define BSWAP(VAL) _byteswap_ulong(VAL)
#else
#define BSWAP(VAL) __builtin_bswap32(VAL)
#endif

		uint32_t ExtractFromHashAtPosition(const Hash256& hash, size_t index) {
			return BSWAP(*reinterpret_cast<const uint32_t*>(hash.data() + index));
		}

		GenerationHashInfo ExtractGenerationHashInfo(const Hash256& generationHash) {
			auto numLeadingZeros = NumLeadingZeros(generationHash);
			if (224 <= numLeadingZeros)
				return GenerationHashInfo{ ExtractFromHashAtPosition(generationHash, Hash256_Size - 4), 224 };

			auto quotient = numLeadingZeros / 8;
			auto remainder = numLeadingZeros % 8;
			auto value = ExtractFromHashAtPosition(generationHash, quotient);
			value <<= remainder;
			value += generationHash[quotient + 4] >> (8 - remainder);
			return GenerationHashInfo{ value, numLeadingZeros };
		}

		/// Calculates the hit for a \a generationHash.
		uint64_t CalculateHit(const Hash256& generationHash) {
			// we want to calculate 2^54 * abs(log(x)), where x = value/2^256 and value is a 256 bit integer
			// note that x is always < 1, therefore log(x) is always negative
			// the original version used boost::multiprecision to convert the generation hash (interpreted as 256 bit integer) to a double
			// the new version uses only the 32 bits beginning at the first non-zero bit of the hash
			// this results in a slightly less exact calculation but the difference is less than one ppm
			auto hashInfo = ExtractGenerationHashInfo(generationHash);

			// handle edge cases
			if (0 == hashInfo.Value)
				return std::numeric_limits<uint64_t>::max();

			if (0xFFFFFFFF == hashInfo.Value)
				return 0;

			// calculate nearest integer for log2(value) * 2 ^ 54
			auto logValue = utils::Log2TimesPowerOfTwo(hashInfo.Value, 54);

			// result is 256 * 2^54 - logValue - (256 - 32 - hashInfo.NumLeadingZeros) * 2^54 which can be simplified
			boost::multiprecision::uint128_t result = (32 + hashInfo.NumLeadingZeros) * Two_To_54 - logValue;

			// divide by log2(e)
			result = result * 10'000'000'000'000'000ull / 14'426'950'408'889'634ull;
			return result.convert_to<uint64_t>();
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

	namespace {
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
				const state::AccountState& accountState) {
			BlockTarget target = baseTarget * ElapsedTime.seconds();
			constexpr Amount OLD_BALANCE{0};
			target *= std::min(accountState.Balances.get(Xpx_Id).unwrap(), OLD_BALANCE.unwrap()); // TODO: replace with actual old balance.
			return target;
		}
	}

	bool BlockHitPredicate::operator()(const Hash256& generationHash, const BlockTarget& parentBaseTarget,
			const utils::TimeSpan& ElapsedTime, const state::AccountState& accountState) const {
		auto hit = CalculateHit(generationHash);
		auto target = CalculateTarget(parentBaseTarget, ElapsedTime, accountState);
		return hit < target;
	}

	bool BlockHitPredicate::operator()(const BlockHitContext& context) const {
		auto hit = CalculateHit(context.GenerationHash);
		auto target = CalculateTarget(context.BaseTarget, context.ElapsedTime, context.AccountState);
		return hit < target;
	}
}}
