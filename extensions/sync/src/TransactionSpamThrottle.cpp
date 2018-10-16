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
#include "catapult/cache/MemoryUtCache.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/BalanceView.h"

namespace catapult { namespace sync {

	namespace {
		Amount GetUsefulBalance(Amount fee, Amount balance, const SpamThrottleConfiguration& config) {
			// maxBalanceBoost <= 9 * 10 ^ 7, so maxBalanceBoost * maxFee should not overflow
			uint64_t maxBalanceBoost = config.TotalBalance.unwrap() / 100u;
			Amount maxFee = std::min(config.MaxBoostFee, fee);
			uint64_t attemptedBalanceBoost = maxBalanceBoost * maxFee.unwrap() / config.MaxBoostFee.unwrap();
			return balance + Amount(attemptedBalanceBoost);
		}

		size_t GetMaxTransactions(size_t cacheSize, size_t maxCacheSize, Amount usefulBalance, Amount totalBalance) {
			auto slotsLeft = maxCacheSize - cacheSize;
			double scaleFactor = std::exp(-3.0 * cacheSize / maxCacheSize);

			// the value 100 is empirical and thus has no special meaning
			return static_cast<size_t>(scaleFactor * usefulBalance.unwrap() * slotsLeft * 100 / totalBalance.unwrap());
		}

		class TransactionSpamThrottle {
		private:
			using TransactionSource = chain::UtUpdater::TransactionSource;

		public:
			explicit TransactionSpamThrottle(const SpamThrottleConfiguration& config, const predicate<const model::Transaction&>& isBonded)
					: m_config(config)
					, m_isBonded(isBonded)
			{}

		public:
			bool operator()(const model::TransactionInfo& transactionInfo, const chain::UtUpdater::ThrottleContext& context) const {
				auto cacheSize = context.TransactionsCache.size();

				// always reject if cache is completely full
				if (cacheSize >= m_config.MaxCacheSize)
					return true;

				// do not apply throttle unless cache contains more transactions than can fit in a single block
				if (m_config.MaxBlockSize > cacheSize)
					return false;

				// bonded transactions and transactions originating from reverted blocks do not get rejected
				if (m_isBonded(*transactionInfo.pEntity) || TransactionSource::Reverted == context.TransactionSource)
					return false;

				const auto& signer = transactionInfo.pEntity->Signer;
				auto readOnlyAccountStateCache = context.UnconfirmedCatapultCache.sub<cache::AccountStateCache>();
				cache::BalanceView balanceView(readOnlyAccountStateCache);
				auto balance = balanceView.getEffectiveBalance(signer);
				auto usefulBalance = GetUsefulBalance(transactionInfo.pEntity->Fee, balance, m_config);
				auto maxTransactions = GetMaxTransactions(cacheSize, m_config.MaxCacheSize, usefulBalance, m_config.TotalBalance);
				return context.TransactionsCache.count(signer) >= maxTransactions;
			}

		private:
			SpamThrottleConfiguration m_config;
			predicate<const model::Transaction&> m_isBonded;
		};
	}

	chain::UtUpdater::Throttle CreateTransactionSpamThrottle(
			const SpamThrottleConfiguration& config,
			const predicate<const model::Transaction&>& isBonded) {
		return TransactionSpamThrottle(config, isBonded);
	}
}}
