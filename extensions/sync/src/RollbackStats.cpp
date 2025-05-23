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

#include "RollbackStats.h"

namespace catapult { namespace sync {

	RollbackStats::RollbackStats()
			: m_totalRollbacks(0)
			, m_longestRollback(0)
	{}

	RollbackStats::RollbackStats(const RollbackStats& other) {
		*this = other;
	}

	RollbackStats& RollbackStats::operator=(const RollbackStats& other){
		// std::atomic has no copy constructor, so here
		// we atomically load from old atomic, and then store into new atomic
		m_totalRollbacks = (size_t)other.m_totalRollbacks;
		m_rollbackSizes = other.m_rollbackSizes;
		m_longestRollback = other.m_longestRollback;
		return *this;
	}

	uint64_t RollbackStats::total(RollbackCounterType rollbackCounterType) const {
		switch (rollbackCounterType) {
			case RollbackCounterType::All:
				return m_totalRollbacks;

			case RollbackCounterType::Recent:
				return m_rollbackSizes.size();

			case RollbackCounterType::Longest:
				return m_longestRollback;
		}

		return 0;
	}

	void RollbackStats::add(Timestamp timestamp, size_t rollbackSize) {
		if (0 == rollbackSize)
			return;

		m_rollbackSizes.push_back({ std::move(timestamp), rollbackSize });
		m_longestRollback = std::max<uint64_t>(m_longestRollback, rollbackSize);
		++m_totalRollbacks;
	}

	void RollbackStats::prune(const Timestamp& threshold) {
		auto iter = m_rollbackSizes.begin();
		while (iter != m_rollbackSizes.end()) {
			if (iter->Timestamp > threshold)
				break;

			iter = m_rollbackSizes.erase(iter);
		}
	}
}}
