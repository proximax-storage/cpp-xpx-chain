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

#include "RollbackInfo.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/model/Elements.h"

namespace catapult { namespace sync {

	RollbackInfo::RollbackInfo(const chain::TimeSupplier& timeSupplier, extensions::ServiceState& state)
			: m_timeSupplier(timeSupplier)
			, m_state(state)
			, m_currentRollbackSize(0)
	{}

	size_t RollbackInfo::counter(RollbackResult rollbackResult, RollbackCounterType rollbackCounterType) const {
		return RollbackResult::Committed == rollbackResult
				? m_committed.total(rollbackCounterType)
				: m_ignored.total(rollbackCounterType);
	}

	void RollbackInfo::increment() {
		++m_currentRollbackSize;
	}

	void RollbackInfo::reset() {
		m_ignored.add(m_timeSupplier(), m_currentRollbackSize);
		m_currentRollbackSize = 0;

		prune();
	}

	void RollbackInfo::save() {
		m_committed.add(m_timeSupplier(), m_currentRollbackSize);
		m_currentRollbackSize = 0;

		prune();
	}

	void RollbackInfo::prune() {
		auto rollbackDurationFull = CalculateFullRollbackDuration(m_state.config().BlockChain);
		auto recentStatsTimeSpan = utils::TimeSpan::FromMilliseconds(rollbackDurationFull.millis() / 2);
		auto threshold = utils::SubtractNonNegative(m_timeSupplier(), recentStatsTimeSpan);

		m_committed.prune(threshold);
		m_ignored.prune(threshold);
	}
}}
