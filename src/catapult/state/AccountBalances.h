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
#include "catapult/model/BlockChainConfiguration.h"
#include "catapult/types.h"
#include "catapult/utils/Hashers.h"
#include "CompactMosaicUnorderedMap.h"
#include <list>

namespace catapult { namespace state {
	struct AccountState;

	/// Container holding information about account.
	class AccountBalances {
	public:
		/// Creates an empty account balances.
		explicit AccountBalances(AccountState* accountState);

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
		const std::list<model::BalanceSnapshot>& snapshots() const {
			return m_localSnapshots;
		}

		/// Add snapshots to snapshots
		void addSnapshot(const model::BalanceSnapshot& snapshot) {
			return m_localSnapshots.push_back(snapshot);
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
		/// Adds \a amount funds to a given mosaic (\a mosaicId) without update of snapshots.
		AccountBalances& credit(const MosaicId& mosaicId, const Amount& amount);

		/// Subtracts \a amount funds from a given mosaic (\a mosaicId) without update of snapshots.
		AccountBalances& debit(const MosaicId& mosaicId, const Amount& amount);

		/// Adds \a amount funds to a given mosaic (\a mosaicId) at \a height.
		AccountBalances& credit(const MosaicId& mosaicId, const Amount& amount, const Height& height);

		/// Subtracts \a amount funds from a given mosaic (\a mosaicId) at \a height.
		AccountBalances& debit(const MosaicId& mosaicId, const Amount& amount, const Height& height);

		/// Commit snapshots from m_remoteSnapshots queue to m_localSnapshots queue
		/// During commit we can remove snapshots from front of m_localSnapshots, to have valid history of account
		void commitSnapshots();

		/// Check do we need to clean up the deque at \a height with \a config
		void maybeCleanUpSnapshots(const Height& height, const model::BlockChainConfiguration config);

		/// Get effective balance of account at \a height with \a importanceGrouping
		Amount getEffectiveBalance(const Height& height, const uint64_t& importanceGrouping) const;

	private:
		/// Maybe push snapshot to deque during commit by \a mosaicId, new \a amount of xpx at \a height.
		void maybePushSnapshot(const MosaicId& mosaicId, const Amount& amount, const Height& height);

		/// Push snapshot to deque
		void pushSnapshot(const model::BalanceSnapshot& snapshot, bool committed = false);

		/// Adds \a amount funds to a given mosaic (\a mosaicId) at \a height.
		AccountBalances& internalCredit(const MosaicId& mosaicId, const Amount& amount, const Height& height);

		/// Subtracts \a amount funds from a given mosaic (\a mosaicId) at \a height.
		AccountBalances& internalDebit(const MosaicId& mosaicId, const Amount& amount, const Height& height);

	private:
		CompactMosaicUnorderedMap m_balances;
		std::list<model::BalanceSnapshot> m_localSnapshots;
		std::list<model::BalanceSnapshot> m_remoteSnapshots;
		AccountState* m_accountState = nullptr;
	};
}}
