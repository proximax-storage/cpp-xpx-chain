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
#include "catapult/model/BlockUtils.h"
#include "catapult/model/Address.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/utils/IntegerMath.h"
#include "catapult/utils/TimeSpan.h"

namespace catapult { namespace chain {

	namespace {
		constexpr uint64_t GAMMA_NUMERATOR{64};
		constexpr uint64_t GAMMA_DENOMINATOR{100};
		constexpr uint32_t SMOOTHING_FACTOR_DENOMINATOR{1000};
	}

	uint64_t CalculateHit(const Hash256& generationHash) {
		return *reinterpret_cast<const uint64_t*>(generationHash.data());
	}

	BlockTarget CalculateTarget(
			const BlockTarget& baseTarget,
			const utils::TimeSpan& elapsedTime,
			const Amount& effectiveBalance) {
		return baseTarget * elapsedTime.seconds() * effectiveBalance.unwrap();
	}

	BlockTarget CalculateBaseTarget(
			const BlockTarget& Tp,
			const utils::TimeSpan& milliSeconds,
			const model::BlockChainConfiguration& config) {
		auto S = milliSeconds.seconds();
		auto RATIO = config.BlockGenerationTargetTime.seconds();
		if (RATIO <= 0)
		{
			CATAPULT_THROW_INVALID_ARGUMENT("BlockGenerationTargetTime is invalid or not set");
		}
		auto factor = config.BlockTimeSmoothingFactor / SMOOTHING_FACTOR_DENOMINATOR;
		auto MINRATIO = RATIO - factor;
		auto MAXRATIO = RATIO + factor;
		if (S > RATIO) {
			return Tp * std::min(S, MAXRATIO) / RATIO;
		} else {
			return Tp - Tp * GAMMA_NUMERATOR * (RATIO - std::max(S, MINRATIO) ) / GAMMA_DENOMINATOR / RATIO;
		}
	}

	inline Amount getXpxOfAccount(const cache::ReadOnlyAccountStateCache& cache, const Key& signer) {
		auto pAccount = cache.tryGet(signer);

		if (!pAccount) {
			pAccount = cache.tryGet(model::PublicKeyToAddress(signer, cache.networkIdentifier()));
		}

		if (pAccount) {
			return pAccount->Balances.get(Xpx_Id);
		}

		return Amount(0);
	}

	Amount CalculateEffectiveBalance(
			const cache::ReadOnlyAccountStateCache& currentCache,
			const cache::ReadOnlyAccountStateCache& previousCache,
			const Height& effectiveBalanceHeight,
			const Height& currentHeight,
			const Key& signer) {
		if (currentHeight <= effectiveBalanceHeight) {
			return getXpxOfAccount(currentCache, signer);
		} else {
			return std::min(getXpxOfAccount(currentCache, signer), getXpxOfAccount(previousCache, signer));
		}
	}

	bool BlockHitPredicate::operator()(const Hash256& generationHash, const BlockTarget& baseTarget,
			const utils::TimeSpan& elapsedTime, const Amount& effectiveBalance) const {
		auto hit = CalculateHit(generationHash);
		auto target = CalculateTarget(baseTarget, elapsedTime, effectiveBalance);
		return hit < target;
	}

	bool BlockHitPredicate::operator()(const model::BlockHitContext& context) const {
		auto hit = CalculateHit(context.GenerationHash);
		auto target = CalculateTarget(context.BaseTarget, context.ElapsedTime, context.EffectiveBalance);
		return hit < target;
	}
}}
