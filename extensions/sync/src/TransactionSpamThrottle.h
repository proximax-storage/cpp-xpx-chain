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
#include "catapult/chain/UtUpdater.h"

namespace catapult { namespace sync {

	/// Spam throttle configuration.
	struct SpamThrottleConfiguration {
	public:
		/// Creates a default spam throttle configuration.
		SpamThrottleConfiguration()
				: TotalBalance(0)
				, MaxCacheSize(0)
				, MaxBlockSize(0)
		{}

		/// Creates a spam throttle configuration around \a maxBoostFee, \a totalBalance, \a maxCacheSize and \a maxBlockSize.
		SpamThrottleConfiguration(Amount maxBoostFee, Amount totalBalance, uint32_t maxCacheSize, uint32_t maxBlockSize)
				: MaxBoostFee(maxBoostFee)
				, TotalBalance(totalBalance)
				, MaxCacheSize(maxCacheSize)
				, MaxBlockSize(maxBlockSize)
		{}

	public:
		/// Max fee for boosting balance.
		Amount MaxBoostFee;

		/// Total balance of all accounts.
		Amount TotalBalance;

		/// Maximum transactions cache size.
		uint32_t MaxCacheSize;

		/// Maximum block size.
		uint32_t MaxBlockSize;
	};

	/// Creates a throttle using \a config to filter out transactions that are considered to be spam.
	/// \a isBonded indicates whether a transaction is bonded or not.
	chain::UtUpdater::Throttle CreateTransactionSpamThrottle(
			const SpamThrottleConfiguration& config,
			const predicate<const model::Transaction&>& isBonded);
}}
