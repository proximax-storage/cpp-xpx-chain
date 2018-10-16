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

#include "constants.h"
#include "timesync/src/filters/AggregateSynchronizationFilter.h"
#include "types.h"
#include <src/catapult/extensions/LocalNodeStateRef.h>

namespace catapult {
	namespace cache {
		class AccountStateCacheView;
		class BalanceView;
	}
}

namespace catapult { namespace timesync {

	/// A time synchronizer that synchronizes local time with the network.
	class TimeSynchronizer {
	public:
		/// Creates a time synchronizer around \a filter, \a totalChainBalance and \a warningThresholdMillis.
		explicit TimeSynchronizer(
				const filters::AggregateSynchronizationFilter& filter,
				uint64_t totalChainBalance,
				int64_t warningThresholdMillis = Warning_Threshold_Millis);

	public:
		/// Calculates a time offset from \a samples using \a accountStateCacheView and \a nodeAge.
		TimeOffset calculateTimeOffset(
				const extensions::LocalNodeStateRef& localNodeState,
				TimeSynchronizationSamples&& samples,
				NodeAge nodeAge);

	private:
		Amount::ValueType sumBalances(
				const cache::BalanceView& view,
				const TimeSynchronizationSamples& samples);

		double sumScaledOffsets(
				const cache::BalanceView& view,
				const TimeSynchronizationSamples& samples,
				double scaling);
	private:
		filters::AggregateSynchronizationFilter m_filter;
		uint64_t m_totalChainBalance;
		int64_t m_warningThresholdMillis;
	};
}}
