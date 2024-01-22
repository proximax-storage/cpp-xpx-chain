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
#include "CompactMosaicMap.h"
#include "catapult/utils/Hashers.h"
#include "catapult/exceptions.h"
#include "catapult/model/BalanceSnapshot.h"
#include "catapult/model/NetworkConfiguration.h"
#include "catapult/types.h"
#include "catapult/utils/Hashers.h"
#include "MosaicUnlockRequest.h"
#include <boost/iterator/zip_iterator.hpp>
#include <list>
#include <set>
#include "catapult/utils/SimpleMutationTracker.h"

namespace catapult { namespace state {
	struct AccountState;

	enum class AccountBalancesTrackables:size_t {
		Unlocked = 0,
		Locked,
		Max
	};

	struct StableBalanceSnapshot {
		uint64_t ImportanceGrouping;
		uint32_t MaxRollbackBlocks;
		StableBalanceSnapshot(
				const uint64_t importanceGrouping,
				const uint32_t maxRollbackBlocks);
		Height GetUnstableHeight(int multiplier = 1) const;
	};

	/// Container holding information about account.
	class AccountBalances {
	private:
		template<bool ProperEffectiveBalanceSorting>
		const model::BalanceSnapshot* ReferenceSnapshot(const Height& effectiveHeight) const;
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

		const CompactMosaicMap& balances() const
		{
			return m_balances;
		}

		const CompactMosaicMap& lockedBalances() const
		{
			return m_lockedBalances;
		}
		/// Returns const ref to snapshots.
		const std::list<model::BalanceSnapshot>& snapshots() const {
			return m_localSnapshots;
		}

		/// Add snapshots to snapshots
		void addSnapshot(const model::BalanceSnapshot& snapshot) {
			return pushSnapshot(snapshot, true /* committed */);
		}

		model::BalancePair GetTrackedBalance() const;

		/// Gets the optimized mosaic id.
		MosaicId optimizedMosaicId() const;

		/// Gets the tracked mosaic id.
		MosaicId trackedMosaicId() const;

		/// Returns amount of funds of a given mosaic (\a mosaicId).
		Amount get(MosaicId mosaicId) const;

		/// Returns total amount of mosaics in this balances (\a mosaicId).
		size_t size() const;

		/// Returns amount of locked funds of a given mosaic (\a mosaicId).
		Amount getLocked(MosaicId mosaicId) const;

	public:
		/// Steals all the balances from the target /a balances
		void steal(AccountBalances& balances, AccountState& newAccount);

		/// Adds \a amount funds to a given mosaic (\a mosaicId).
		/// It will increase balance of account without tracking of it in snapshots array.
		AccountBalances& credit(const MosaicId& mosaicId, const Amount& amount);

		/// Subtracts \a amount funds from a given mosaic (\a mosaicId).
		/// It will decrease balance of account without tracking of it in snapshots array.
		AccountBalances& debit(const MosaicId& mosaicId, const Amount& amount);

		/// Locks \a amount funds to a given mosaic (\a mosaicId).
		/// It will increase balance of account without tracking of it in snapshots array.
		AccountBalances& lock(const MosaicId& mosaicId, const Amount& amount);

		/// Unlocks \a amount funds from a given mosaic (\a mosaicId).
		/// It will decrease balance of account without tracking of it in snapshots array.
		AccountBalances& unlock(const MosaicId& mosaicId, const Amount& amount);

		/// Adds \a amount funds to a given mosaic (\a mosaicId) at \a height.
		/// Increasing of XPX balance will be tracked in snapshots array.
		AccountBalances& credit(const MosaicId& mosaicId, const Amount& amount, const Height& height);

		/// Subtracts \a amount funds from a given mosaic (\a mosaicId) at \a height.
		/// Decreasing of XPX balance will be tracked in snapshots array.
		AccountBalances& debit(const MosaicId& mosaicId, const Amount& amount, const Height& height);

		/// Locks \a amount funds to a given mosaic (\a mosaicId).
		/// It will increase balance of account without tracking of it in snapshots array.
		AccountBalances& lock(const MosaicId& mosaicId, const Amount& amount, const Height& height);

		/// Unlocks \a amount funds from a given mosaic (\a mosaicId).
		/// It will decrease balance of account without tracking of it in snapshots array.
		AccountBalances& unlock(const MosaicId& mosaicId, const Amount& amount, const Height& height);

		/// Unlocks \a amount funds from a given mosaic (\a mosaicId).
		/// It will decrease balance of account without tracking of it in snapshots array.
		AccountBalances& requestUnlock(const MosaicId& mosaicId, const Amount& amount, const Height& height);

		/// Optimizes access of the mosaic with \a id.
		void optimize(MosaicId id);

		/// Trackes the mosaic with \a id.
		void track(MosaicId id);

		/// Commit snapshots from m_remoteSnapshots queue to m_localSnapshots queue
		/// During commit we can remove snapshots from front of m_localSnapshots, to have valid history of account
		void commitSnapshots();

		/// Remove all snapshots
		void cleanUpSnaphots() {
			m_remoteSnapshots.clear();
			m_localSnapshots.clear();
		}

		/// Check do we need to clean up the deque at \a height with \a holder and \a configHeights
		void maybeCleanUpSnapshots(const Height& height, const model::NetworkConfiguration& config, const StableBalanceSnapshot& unstableHeight);

		/// Get effective balance of account at \a height with \a importanceGrouping
		template<bool ProperEffectiveBalanceSorting>
		Amount getEffectiveBalance(const Height& height, const uint64_t& importanceGrouping) const;

		/// Get effective balance of account at \a height with \a importanceGrouping
		template<bool ProperEffectiveBalanceSorting>
		model::BalancePair getCompoundEffectiveBalance(const Height& height, const uint64_t& importanceGrouping) const;

		/// Clears all changes
		void clearChanges();

		/// Returns a const reference to the changes tracker of this balances.
		const utils::SimpleMutationTracker<AccountBalancesTrackables>& changes() const;

	private:
		/// Maybe push snapshot to deque during commit by \a mosaicId, new \a amount of xpx at \a height. Template argument defaults to unlocked balance in order to preserve unit tests, for backwards compatibility.
		template<AccountBalancesTrackables TChangedValue = AccountBalancesTrackables::Unlocked>
		void maybePushSnapshot(const MosaicId& mosaicId, const Amount& amount, const Amount& lockedAmount, const Height& height){
			if (mosaicId != m_trackedMosaicId || height == Height(0) || height == Height(-1)) {
				return;
			}
			m_changesTracker.template mark<TChangedValue>();
			if (m_remoteSnapshots.empty()) {
				pushSnapshot(model::BalanceSnapshot{amount, lockedAmount, height});
				return;
			}

			if (height <= m_remoteSnapshots.back().BalanceHeight) {
				while(!m_remoteSnapshots.empty() &&  m_remoteSnapshots.back().BalanceHeight >= height) {
					m_remoteSnapshots.pop_back();
				}
				pushSnapshot(model::BalanceSnapshot{amount, lockedAmount, height});
			} else if (height > m_remoteSnapshots.back().BalanceHeight) {
				pushSnapshot(model::BalanceSnapshot{amount, lockedAmount, height});
			}
		}


		/// Push snapshot to deque
		void pushSnapshot(const model::BalanceSnapshot& snapshot, bool committed = false);

		/// Adds \a amount funds to a given mosaic (\a mosaicId) at \a height.
		AccountBalances& internalCredit(const MosaicId& mosaicId, const Amount& amount, const Height& height);

		/// Subtracts \a amount funds from a given mosaic (\a mosaicId) at \a height.
		AccountBalances& internalDebit(const MosaicId& mosaicId, const Amount& amount, const Height& height);

		/// Locks \a amount funds to a given mosaic (\a mosaicId) at \a height.
		AccountBalances& internalLock(const MosaicId& mosaicId, const Amount& amount, const Height& height);

		/// Unlocks \a amount funds from a given mosaic (\a mosaicId) at \a height.
		AccountBalances& internalUnlock(const MosaicId& mosaicId, const Amount& amount, const Height& height);

	private:
		std::list<model::BalanceSnapshot> m_localSnapshots;
		std::list<model::BalanceSnapshot> m_remoteSnapshots;
		AccountState* m_accountState = nullptr;
		CompactMosaicMap m_balances;
		CompactMosaicMap m_lockedBalances;
		MosaicId m_optimizedMosaicId;
		MosaicId m_trackedMosaicId;
		utils::SimpleMutationTracker<AccountBalancesTrackables> m_changesTracker;

	};
}}
