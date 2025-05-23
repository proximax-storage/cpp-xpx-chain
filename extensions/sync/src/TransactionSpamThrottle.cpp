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

#include "TransactionSpamThrottle.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/ImportanceView.h"
#include "catapult/extensions/ServiceState.h"

namespace catapult { namespace sync {

	namespace {
		Importance GetEffectiveImportance(Amount fee, Importance importance, const SpamThrottleConfiguration& config) {
			// maxImportanceBoost <= 9 * 10 ^ 7, so maxImportanceBoost * maxFee should not overflow
			uint64_t maxImportanceBoost = config.TotalImportance.unwrap() / 100u;
			Amount maxFee = std::min(config.MaxBoostFee, fee);
			uint64_t attemptedImportanceBoost = maxImportanceBoost * maxFee.unwrap() / config.MaxBoostFee.unwrap();
			return importance + Importance(attemptedImportanceBoost);
		}

		size_t GetMaxTransactions(size_t cacheSize, size_t maxCacheSize, Importance effectiveImportance, Importance totalImportance) {
			auto slotsLeft = maxCacheSize - cacheSize;
			double scaleFactor = std::exp(-3.0 * cacheSize / maxCacheSize);

			// the value 100 is empirical and thus has no special meaning
			return static_cast<size_t>(scaleFactor * effectiveImportance.unwrap() * slotsLeft * 100 / totalImportance.unwrap());
		}

		class TransactionSpamThrottle {
		private:
			using TransactionSource = chain::UtUpdater::TransactionSource;

		public:
			explicit TransactionSpamThrottle(extensions::ServiceState& state, const predicate<const model::Transaction&>& isBonded)
					: m_state(state)
					, m_isBonded(isBonded)
			{}

		public:
			bool operator()(const model::TransactionInfo& transactionInfo, const chain::UtUpdater::ThrottleContext& context) const {
				auto cacheSize = context.TransactionsCache.size();
				const auto& config = m_state.config(context.CacheHeight);
				SpamThrottleConfiguration throttleConfig(
					config.Node.TransactionSpamThrottlingMaxBoostFee,
					config.Network.TotalChainImportance,
					config.Node.UnconfirmedTransactionsCacheMaxSize,
					config.Network.MaxTransactionsPerBlock);

				// always reject if cache is completely full
				if (cacheSize >= throttleConfig.MaxCacheSize)
					return true;

				// do not apply throttle unless cache contains more transactions than can fit in a single block
				if (throttleConfig.MaxBlockSize > cacheSize)
					return false;

				// bonded transactions and transactions originating from reverted blocks do not get rejected
				if (m_isBonded(*transactionInfo.pEntity) || TransactionSource::Reverted == context.TransactionSource)
					return false;

				const auto& signer = transactionInfo.pEntity->Signer;
				auto readOnlyAccountStateCache = context.UnconfirmedCatapultCache.sub<cache::AccountStateCache>();
				cache::ImportanceView importanceView(readOnlyAccountStateCache);
				auto importance = importanceView.getAccountImportanceOrDefault(signer, context.CacheHeight);
				auto effectiveImportance = GetEffectiveImportance(transactionInfo.pEntity->MaxFee, importance, throttleConfig);
				auto maxTransactions = GetMaxTransactions(cacheSize, throttleConfig.MaxCacheSize, effectiveImportance, throttleConfig.TotalImportance);
				return context.TransactionsCache.count(signer) >= maxTransactions;
			}

		private:
			extensions::ServiceState& m_state;
			predicate<const model::Transaction&> m_isBonded;
		};
	}

	chain::UtUpdater::Throttle CreateTransactionSpamThrottle(
			extensions::ServiceState& state,
			const predicate<const model::Transaction&>& isBonded) {
		return TransactionSpamThrottle(state, isBonded);
	}
}}
