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
#include "catapult/types.h"
#include <list>
#include <atomic>

namespace catapult { namespace sync {

	/// Rollback counter types.
	enum class RollbackCounterType {
		/// Number of rollbacks since start of the server.
		All,

		/// Number of rollbacks within a time frame (configuration dependent).
		Recent,

		/// Longest rollback.
		Longest
	};

	/// Container for rollback statistics.
	class RollbackStats {
	public:
		/// Creates rollback statistics container.
		RollbackStats();

		RollbackStats(const RollbackStats& other);

		RollbackStats& operator=(const RollbackStats& other);

	public:
		/// Returns statistics for a type (\a rollbackCounterType).
		uint64_t total(RollbackCounterType rollbackCounterType) const;

		/// Adds info about \a rollbackSize at \a timestamp to current statistics.
		void add(Timestamp timestamp, size_t rollbackSize);

		/// Prunes statistics below time \a threshold.
		void prune(const Timestamp& threshold);

	private:
		struct RollbackStatsEntry {
			catapult::Timestamp Timestamp;
			size_t RollbackSize{};
		};

	private:
		std::atomic<uint64_t> m_totalRollbacks;
		std::list<RollbackStatsEntry> m_rollbackSizes;
		uint64_t m_longestRollback;
	};
}}
