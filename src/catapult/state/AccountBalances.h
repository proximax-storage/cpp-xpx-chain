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
#include "catapult/exceptions.h"
#include "catapult/model/BalanceSnapshot.h"
#include "catapult/types.h"
#include "catapult/utils/Hashers.h"
#include "CompactMosaicUnorderedMap.h"
#include <deque>
#include <unordered_map>

namespace catapult { namespace state {

	/// Container holding information about account.
	class AccountBalances {
	public:
		/// Creates an empty account balances.
		AccountBalances();

		/// Copy constructor that makes a deep copy of \a accountBalances.
		AccountBalances(const AccountBalances& accountBalances);

		/// Move constructor that move constructs an account balances from \a accountBalances.
		AccountBalances(AccountBalances&& accountBalances);

	public:
		/// Assignment operator that makes a deep copy of \a accountBalances.
		AccountBalances& operator=(const AccountBalances& accountBalances);

		/// Move assignment operator that assigns \a accountBalances.
		AccountBalances& operator=(AccountBalances&& accountBalances);

	public:
		/// Returns const ref to snapshots.
		const std::deque<model::BalanceSnapshot>& getSnapshots() const {
			return m_snapshots;
		}

		/// Returns ref to snapshots.
		std::deque<model::BalanceSnapshot>& getSnapshots() {
			return m_snapshots;
		}

		/// Returns the number of mosaics owned.
		size_t size() const {
			return m_balances.size();
		}

		/// Returns a const iterator to the first element of the underlying set.
		auto begin() const {
			return m_balances.begin();
		}

		/// Returns a const iterator to the element following the last element of the underlying set.
		auto end() const {
			return m_balances.end();
		}

		/// Returns amount of funds of a given mosaic (\a mosaicId).
		Amount get(MosaicId mosaicId) const;

	public:
		/// Adds \a amount funds to a given mosaic (\a mosaicId).
		AccountBalances& credit(MosaicId mosaicId, Amount amount);

		/// Subtracts \a amount funds from a given mosaic (\a mosaicId).
		AccountBalances& debit(MosaicId mosaicId, Amount amount);

	private:
		CompactMosaicUnorderedMap m_balances;
		std::deque<model::BalanceSnapshot> m_snapshots;
	};
}}
