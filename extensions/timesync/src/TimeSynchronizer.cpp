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

#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/AccountStateCacheView.h"
#include "catapult/cache_core/BalanceView.h"
#include "catapult/utils/Functional.h"
#include "constants.h"
#include "timesync/src/filters/filter_constants.h"
#include "TimeSynchronizer.h"
#include <cmath>

namespace catapult { namespace timesync {

	namespace {
		constexpr utils::LogLevel MapToLogLevel(int64_t warningThresholdMillis, int64_t offset) {
			return warningThresholdMillis > offset ? utils::LogLevel::Trace : utils::LogLevel::Warning;
		}

		double GetCoupling(NodeAge nodeAge) {
			auto ageToUse = std::max<int64_t>(nodeAge.unwrap() - filters::Start_Decay_After_Round, 0);
			return std::max(std::exp(-Coupling_Decay_Strength * ageToUse) * Coupling_Start, Coupling_Minimum);
		}
	}

	TimeSynchronizer::TimeSynchronizer(
			const filters::AggregateSynchronizationFilter& filter,
			uint64_t totalChainBalance,
			int64_t warningThresholdMillis)
			: m_filter(filter)
			, m_totalChainBalance(totalChainBalance)
			, m_warningThresholdMillis(warningThresholdMillis)
	{}

	TimeOffset TimeSynchronizer::calculateTimeOffset(
			const extensions::LocalNodeStateRef& localNodeState,
			TimeSynchronizationSamples&& samples,
			NodeAge nodeAge) {
		m_filter(samples, nodeAge);
		if (samples.empty()) {
			CATAPULT_LOG(warning) << "no synchronization samples available to calculate network time";
			return TimeOffset(0);
		}

		auto cacheView = localNodeState.Cache.createView();
		auto accountStateCacheView = localNodeState.Cache.sub<cache::AccountStateCache>().createView();

		cache::BalanceView balanceView(cache::ReadOnlyAccountStateCache(cacheView.sub<cache::AccountStateCache>()));
		auto cumulativeBalance = sumBalances(balanceView, samples);
		if (0 == cumulativeBalance) {
			CATAPULT_LOG(warning) << "cannot calculate network time, cumulativeBalance is zero";
			return TimeOffset(0);
		}

		auto highValueAddressesSize = accountStateCacheView->highValueAddressesSize();
		auto viewPercentage = static_cast<double>(samples.size()) / highValueAddressesSize;
		auto balancePercentage = static_cast<double>(cumulativeBalance) / m_totalChainBalance;
		auto scaling = balancePercentage > viewPercentage ? 1.0 / balancePercentage : 1.0 / viewPercentage;
		auto sum = sumScaledOffsets(balanceView, samples, scaling);
		return TimeOffset(static_cast<int64_t>(GetCoupling(nodeAge) * sum));
	}

	Amount::ValueType TimeSynchronizer::sumBalances(
			const cache::BalanceView& view,
			const TimeSynchronizationSamples& samples) {
		return utils::Sum(samples, [&view](const auto& sample) {
			return view.getEffectiveBalance(sample.node().identityKey()).unwrap();
		});
	}

	double TimeSynchronizer::sumScaledOffsets(
			const cache::BalanceView& view,
			const TimeSynchronizationSamples& samples,
			double scaling) {
		auto totalChainBalance = m_totalChainBalance;
		auto warningThresholdMillis = m_warningThresholdMillis;
		return utils::Sum(samples, [&view, scaling, totalChainBalance, warningThresholdMillis](const auto& sample) {
			int64_t offset = sample.timeOffsetToRemote();
			CATAPULT_LOG_LEVEL(MapToLogLevel(warningThresholdMillis, offset))
					<< sample.node().metadata().Name << ": network time offset to local node is " << offset << "ms";

			auto balance = view.getEffectiveBalance(sample.node().identityKey());
			return scaling * offset * balance.unwrap() / totalChainBalance;
		});
	}
}}
