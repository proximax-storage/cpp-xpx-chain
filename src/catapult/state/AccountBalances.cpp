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

#include "AccountBalances.h"
#include "AccountState.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include <unordered_map>

namespace catapult { namespace state {

	namespace {
		static constexpr bool IsZero(Amount amount) {
			return Amount(0) == amount;
		}

		inline auto minSnapshot(const std::list<model::BalanceSnapshot>& snapshots, const Height& height, const uint64_t& importanceGrouping)
		{
			auto importanceGroupingHeight = Height(importanceGrouping);
			auto effectiveHeight = Height(0);

			if (height > importanceGroupingHeight) {
				effectiveHeight = height - importanceGroupingHeight;
			}
			auto minElement = std::min_element(
					snapshots.begin(),
					snapshots.end(),
					[&effectiveHeight](const model::BalanceSnapshot& l, const model::BalanceSnapshot& r){
					  return l.BalanceHeight <= effectiveHeight || l.GetEffectiveAmount() < r.GetEffectiveAmount();
					});

			return minElement;
		}
	}

	AccountBalances::AccountBalances(AccountState* accountState) : m_accountState(accountState) {}

	AccountBalances::AccountBalances(const AccountBalances& accountBalances) {
		*this = accountBalances;
	}

	AccountBalances::AccountBalances(AccountBalances&& accountBalances) = default;

	void AccountBalances::steal(AccountBalances& balances, AccountState& newAccount)
	{
		m_optimizedMosaicId = balances.optimizedMosaicId();
		m_trackedMosaicId = balances.trackedMosaicId();
		m_balances.optimize(m_optimizedMosaicId);
		m_accountState = &newAccount;
		std::vector<MosaicId> toRemove;
		std::vector<MosaicId> toRemoveLocked;

		for (const auto& pair : balances.m_balances)
		{
			m_balances.insert(pair);
			toRemove.push_back(pair.first);
		}

		for (const auto& pair : balances.m_lockedBalances)
		{
			m_lockedBalances.insert(pair);
			toRemoveLocked.push_back(pair.first);
		}
		for (const auto& snapshot : balances.m_localSnapshots)
			pushSnapshot(snapshot, true /* committed */);

		for (const auto& snapshot : balances.m_remoteSnapshots)
			pushSnapshot(snapshot, false /* committed */);

		for(const auto& mosaic : toRemove)
		{
			balances.m_balances.erase(mosaic);
		}

		for(const auto& mosaic : toRemoveLocked)
		{
			balances.m_lockedBalances.erase(mosaic);
		}

		balances.cleanUpSnaphots();


	}
	AccountBalances& AccountBalances::operator=(const AccountBalances& accountBalances) {
		m_optimizedMosaicId = accountBalances.optimizedMosaicId();
		m_trackedMosaicId = accountBalances.trackedMosaicId();
		m_balances.optimize(m_optimizedMosaicId);

		if (!m_accountState)
			m_accountState = accountBalances.m_accountState;

		for (const auto& pair : accountBalances.m_balances)
			m_balances.insert(pair);

		for (const auto& pair : accountBalances.m_lockedBalances)
			m_lockedBalances.insert(pair);

		for (const auto& snapshot : accountBalances.m_localSnapshots)
			pushSnapshot(snapshot, true /* committed */);

		for (const auto& snapshot : accountBalances.m_remoteSnapshots)
			pushSnapshot(snapshot, false /* committed */);

		m_changesTracker = accountBalances.m_changesTracker;
		return *this;
	}

	AccountBalances& AccountBalances::operator=(AccountBalances&& accountBalances) = default;

	MosaicId AccountBalances::optimizedMosaicId() const {
		return m_optimizedMosaicId;
	}

	MosaicId AccountBalances::trackedMosaicId() const {
		return m_trackedMosaicId;
	}

	Amount AccountBalances::get(MosaicId mosaicId) const {
		auto iter = m_balances.find(mosaicId);
		return m_balances.end() == iter ? Amount(0) : iter->second;
	}

	Amount AccountBalances::getLocked(MosaicId mosaicId) const {
		auto iter = m_lockedBalances.find(mosaicId);
		return m_lockedBalances.end() == iter ? Amount(0) : iter->second;
	}

	// Consider optimizing by tracking size! Consider single modified CompactMosaicMap
	size_t AccountBalances::size() const{
		std::unordered_map<uint64_t, bool> tracker;
		for(auto balance : m_balances)
			tracker[balance.first.unwrap()] = true;

		for(auto balance : m_lockedBalances)
			tracker[balance.first.unwrap()] = true;

		return tracker.size();

	}

	AccountBalances& AccountBalances::credit(const MosaicId& mosaicId, const Amount& amount) {
		return internalCredit(mosaicId, amount, Height(0));
	}

	AccountBalances& AccountBalances::debit(const MosaicId& mosaicId, const Amount& amount) {
		return internalDebit(mosaicId, amount, Height(0));
	}

	AccountBalances& AccountBalances::credit(const MosaicId& mosaicId, const Amount& amount, const Height& height) {
		if (height.unwrap() == 0)
			CATAPULT_THROW_RUNTIME_ERROR("height during credit can't be zero");

		return internalCredit(mosaicId, amount, height);
	}

	AccountBalances& AccountBalances::debit(const MosaicId& mosaicId, const Amount& amount, const Height& height) {
		if (height.unwrap() == 0)
			CATAPULT_THROW_RUNTIME_ERROR("height during debit can't be zero");

		return internalDebit(mosaicId, amount, height);
	}

	AccountBalances& AccountBalances::lock(const MosaicId& mosaicId, const Amount& amount) {
		return internalLock(mosaicId, amount, Height(0));
	}

	AccountBalances& AccountBalances::unlock(const MosaicId& mosaicId, const Amount& amount) {
		return internalUnlock(mosaicId, amount, Height(0));
	}

	AccountBalances& AccountBalances::lock(const MosaicId& mosaicId, const Amount& amount, const Height& height) {
		if (height.unwrap() == 0)
		CATAPULT_THROW_RUNTIME_ERROR("height during credit can't be zero");

		return internalLock(mosaicId, amount, height);
	}

	AccountBalances& AccountBalances::unlock(const MosaicId& mosaicId, const Amount& amount, const Height& height) {
		if (height.unwrap() == 0)
		CATAPULT_THROW_RUNTIME_ERROR("height during debit can't be zero");

		return internalUnlock(mosaicId, amount, height);
	}


	AccountBalances& AccountBalances::internalCredit(const MosaicId& mosaicId, const Amount& amount, const Height& height) {
		if(m_accountState && m_accountState->IsLocked())
			CATAPULT_THROW_RUNTIME_ERROR("Locked accounts cannot be modified!");
		if (IsZero(amount))
			return *this;

		auto iter = m_balances.find(mosaicId);
		auto lockedIter = m_lockedBalances.find(mosaicId);
		auto lockedBalance = m_lockedBalances.end() == lockedIter ? Amount(0) : lockedIter->second;

		if (m_balances.end() == iter) {
			if (m_remoteSnapshots.empty() && m_localSnapshots.empty()) {
				maybePushSnapshot<AccountBalancesTrackables::Unlocked>(mosaicId, Amount(0), lockedBalance, height - Height(1));
			}
			m_balances.insert(std::make_pair(mosaicId, amount));
			maybePushSnapshot<AccountBalancesTrackables::Unlocked>(mosaicId, amount, lockedBalance, height);
		} else {
			if (m_remoteSnapshots.empty() && m_localSnapshots.empty()) {
				maybePushSnapshot<AccountBalancesTrackables::Unlocked>(mosaicId, iter->second, lockedBalance, height - Height(1));
			}
			iter->second = iter->second + amount;
			maybePushSnapshot<AccountBalancesTrackables::Unlocked>(mosaicId, iter->second, lockedBalance, height);
		}
		return *this;
	}

	AccountBalances& AccountBalances::internalDebit(const MosaicId& mosaicId, const Amount& amount, const Height& height) {
		if(m_accountState && m_accountState->IsLocked())
			CATAPULT_THROW_RUNTIME_ERROR("Locked accounts cannot be modified!");
		if (IsZero(amount))
			return *this;

		auto iter = m_balances.find(mosaicId);
		auto lockedIter = m_lockedBalances.find(mosaicId);
		auto lockedBalance = m_lockedBalances.end() == lockedIter ? Amount(0) : lockedIter->second;
		auto hasZeroBalance = m_balances.end() == iter;
		if (hasZeroBalance || amount > iter->second) {
			auto currentBalance = hasZeroBalance ? Amount(0) : iter->second;
			std::ostringstream out;
			out
					<< "debit amount (" << amount << ") is greater than current balance (" << currentBalance
					<< ") for mosaic " << utils::HexFormat(mosaicId);
			CATAPULT_THROW_RUNTIME_ERROR(out.str().c_str());
		}

		iter->second = iter->second - amount;

		maybePushSnapshot<AccountBalancesTrackables::Unlocked>(mosaicId, iter->second, lockedBalance,  height);

		if (IsZero(iter->second))
			m_balances.erase(mosaicId);

		return *this;
	}

	AccountBalances& AccountBalances::internalLock(const MosaicId& mosaicId, const Amount& amount, const Height& height) {
		if(m_accountState && m_accountState->IsLocked())
		CATAPULT_THROW_RUNTIME_ERROR("Locked accounts cannot be modified!");
		if (IsZero(amount))
			return *this;

		auto iter = m_balances.find(mosaicId);

		auto lockedIter = m_lockedBalances.find(mosaicId);
		auto hasZeroBalance = m_balances.end() == iter;

		if (hasZeroBalance || amount > iter->second) {
			auto currentBalance = hasZeroBalance ? Amount(0) : iter->second;
			std::ostringstream out;
			out
					<< "lock amount (" << amount << ") is greater than current balance (" << currentBalance
					<< ") for mosaic " << utils::HexFormat(mosaicId);
			CATAPULT_THROW_RUNTIME_ERROR(out.str().c_str());
		}

		auto originalBalance = iter->second;

		iter->second = iter->second - amount;

		if (m_lockedBalances.end() == lockedIter) {
			if (m_remoteSnapshots.empty() && m_localSnapshots.empty()) {
				maybePushSnapshot<AccountBalancesTrackables::Locked>(mosaicId, originalBalance, Amount(0), height - Height(1));
			}
			m_lockedBalances.insert(std::make_pair(mosaicId, amount));
			maybePushSnapshot<AccountBalancesTrackables::Locked>(mosaicId, iter->second, amount, height);

		} else {
			if (m_remoteSnapshots.empty() && m_localSnapshots.empty()) {
				maybePushSnapshot<AccountBalancesTrackables::Locked>(mosaicId, originalBalance, Amount(0), height - Height(1));
			}
			lockedIter->second = lockedIter->second + amount;
			maybePushSnapshot<AccountBalancesTrackables::Locked>(mosaicId, iter->second, lockedIter->second, height);
		}


		if (IsZero(iter->second))
			m_balances.erase(mosaicId);

		return *this;
	}

	AccountBalances& AccountBalances::internalUnlock(const MosaicId& mosaicId, const Amount& amount, const Height& height) {
		if(m_accountState && m_accountState->IsLocked())
		CATAPULT_THROW_RUNTIME_ERROR("Locked accounts cannot be modified!");
		if (IsZero(amount))
			return *this;

		auto iter = m_balances.find(mosaicId);

		auto lockedIter = m_lockedBalances.find(mosaicId);
		auto hasZeroBalance = m_lockedBalances.end() == lockedIter;

		if (hasZeroBalance || amount > lockedIter->second) {
			auto currentBalance = hasZeroBalance ? Amount(0) : lockedIter->second;
			std::ostringstream out;
			out
					<< "unlock amount (" << amount << ") is greater than current balance (" << currentBalance
					<< ") for mosaic " << utils::HexFormat(mosaicId);
			CATAPULT_THROW_RUNTIME_ERROR(out.str().c_str());
		}

		auto originalBalance = lockedIter->second;

		lockedIter->second = lockedIter->second - amount;

		if (m_balances.end() == iter) {
			if (m_remoteSnapshots.empty() && m_localSnapshots.empty()) {
				maybePushSnapshot<AccountBalancesTrackables::Locked>(mosaicId, Amount(0), originalBalance, height - Height(1));
			}
			m_balances.insert(std::make_pair(mosaicId, amount));
			maybePushSnapshot<AccountBalancesTrackables::Locked>(mosaicId, amount, lockedIter->second, height);

		} else {
			if (m_remoteSnapshots.empty() && m_localSnapshots.empty()) {
				maybePushSnapshot<AccountBalancesTrackables::Locked>(mosaicId, Amount(0), originalBalance, height - Height(1));
			}
			iter->second = iter->second + amount;
			maybePushSnapshot<AccountBalancesTrackables::Locked>(mosaicId, iter->second, lockedIter->second, height);
		}


		if (IsZero(lockedIter->second))
			m_lockedBalances.erase(mosaicId);

		return *this;
	}

	void AccountBalances::optimize(MosaicId id) {
		m_balances.optimize(id);
		m_lockedBalances.optimize(id);
		m_optimizedMosaicId = id;
	}

	void AccountBalances::track(MosaicId id) {
		if (m_trackedMosaicId != id && (!m_localSnapshots.empty() || !m_remoteSnapshots.empty()))
			CATAPULT_THROW_RUNTIME_ERROR("can't track another id, if history for previous id is not empty");

		m_trackedMosaicId = id;
	}

	void AccountBalances::commitSnapshots() {
		if (m_remoteSnapshots.empty()) {
			return;
		}

		while (!m_localSnapshots.empty() && m_localSnapshots.back().BalanceHeight >= m_remoteSnapshots.front().BalanceHeight) {
			m_localSnapshots.pop_back();
		}

		while (!m_localSnapshots.empty() && !m_remoteSnapshots.empty()
				&& m_localSnapshots.back().Amount == m_remoteSnapshots.front().Amount
			    && m_localSnapshots.back().LockedAmount == m_remoteSnapshots.front().LockedAmount) {
			m_remoteSnapshots.pop_front();
		}

		m_localSnapshots.splice(m_localSnapshots.end(), m_remoteSnapshots);
	}

	void AccountBalances::maybeCleanUpSnapshots(const Height& height, const model::NetworkConfiguration& activeConfig, const StableBalanceSnapshot& unstableHeight) {

		auto effectiveUnstableHeight = unstableHeight.GetUnstableHeight(activeConfig.ProperEffectiveBalanceCalculation ? 2 : 1);
		if (height <= effectiveUnstableHeight)
			return;

		auto stableHeight = height - effectiveUnstableHeight;
		//Dry run to find out how many snapshots are behind stableHeight;
		if(activeConfig.ProperEffectiveBalanceCalculation) {
			int count = 0;
			for(auto& snapshot : m_localSnapshots) {
				if(m_localSnapshots.front().BalanceHeight <= stableHeight)
					count++;
			}
			while(!m_localSnapshots.empty() && m_localSnapshots.front().BalanceHeight <= stableHeight && count > 1) {
				m_localSnapshots.pop_front();
				count--;
			}
		} else{
			while(!m_localSnapshots.empty() && m_localSnapshots.front().BalanceHeight <= stableHeight) {
				m_localSnapshots.pop_front();
			}
		}
	}

	///TODO: Optimize
	template<>
	const model::BalanceSnapshot* AccountBalances::ReferenceSnapshot<true>(const catapult::Height& effectiveHeight) const{
		///Only the latest snapshot prior to effectiveHeight is kept.
		const model::BalanceSnapshot* referenceSnapshot = nullptr;

		/// In any case, for use cases where a balance might be forced to change in the past for some reason, we will assume remote snapshots take priority.
		/// We use at most the last snapshot prior to effectiveHeight, if that is the lowest that will always represent the effective balance. If it is a remote snapshot it takes priority.

		/// AFTER figuring out which one is the reference we need to figure out which snapshot is the lowest. This is always the effective one regardless.

		for (auto it = m_localSnapshots.rbegin(); it != m_localSnapshots.rend(); ++it) {
			if(!referenceSnapshot || referenceSnapshot->GetEffectiveAmount() > it->GetEffectiveAmount())
				referenceSnapshot = &(*it);
			if(it->BalanceHeight <= effectiveHeight)
				break;
		}
		// If there is a remote snapshot that is lower, we override our selection with it.
		for (auto it = m_remoteSnapshots.rbegin(); it != m_remoteSnapshots.rend(); ++it) {
			if(!referenceSnapshot || referenceSnapshot->GetEffectiveAmount() > it->GetEffectiveAmount())
				referenceSnapshot = &(*it);
			if(it->BalanceHeight <= effectiveHeight)
				break;
		}

		return referenceSnapshot;
	}

	template<>
	const model::BalanceSnapshot* AccountBalances::ReferenceSnapshot<false>(const catapult::Height& effectiveHeight) const{

		const model::BalanceSnapshot* referenceSnapshot = nullptr;

		if(m_remoteSnapshots.empty() && m_localSnapshots.empty())
			return referenceSnapshot;

		auto minElementDepreciator = [&effectiveHeight](const model::BalanceSnapshot& l, const model::BalanceSnapshot& r){
			return l.BalanceHeight <= effectiveHeight || l.GetEffectiveAmount() < r.GetEffectiveAmount();
		};

		if (m_remoteSnapshots.empty()) {
			referenceSnapshot = &*std::min_element(
					m_localSnapshots.begin(),
					m_localSnapshots.end(), minElementDepreciator);
		}  else if (m_localSnapshots.empty()) {
			referenceSnapshot = &*std::min_element(
					m_remoteSnapshots.begin(),
					m_remoteSnapshots.end(),
					minElementDepreciator);
		} else {
			referenceSnapshot = std::min(
					&*std::min_element(m_remoteSnapshots.begin(), m_remoteSnapshots.end(), minElementDepreciator),
					&*std::min_element(m_localSnapshots.begin(), m_localSnapshots.end(), minElementDepreciator),
					[](auto x, auto y) { return x->GetEffectiveAmount() < y->GetEffectiveAmount(); });
		}

		return referenceSnapshot;
	}

	model::BalancePair AccountBalances::GetTrackedBalance() const{
		model::BalancePair result;
		auto iter = m_balances.find(m_trackedMosaicId);
		auto lockedIter = m_lockedBalances.find(m_trackedMosaicId);
		result.Unlocked = m_balances.end() == iter ? Amount(0) : iter->second;
		result.Locked = m_lockedBalances.end() == lockedIter ? Amount(0) : lockedIter->second;
		return result;
	}

	template<bool ProperEffectiveBalanceSorting>
	Amount AccountBalances::getEffectiveBalance(const Height& height, const uint64_t& importanceGrouping) const {
		auto importanceGroupingHeight = Height(importanceGrouping);
		auto effectiveHeight = Height(0);

		if (height > importanceGroupingHeight) {
			effectiveHeight = height - importanceGroupingHeight;
		}

		auto referenceSnapshot = ReferenceSnapshot<ProperEffectiveBalanceSorting>(effectiveHeight);
		/// If there is no reference snapshot. We return balance as there was never an interaction, or it was removed due to previous calculation approach.
		if (!referenceSnapshot) {
			return GetTrackedBalance().Sum();
		}
		/// If there is a reference snapshot then that is our effective balance.
		return referenceSnapshot->GetEffectiveAmount();
	}
	template<bool ProperEffectiveBalanceSorting>
	model::BalancePair AccountBalances::getCompoundEffectiveBalance(const Height& height, const uint64_t& importanceGrouping) const {
		auto importanceGroupingHeight = Height(importanceGrouping);
		auto effectiveHeight = Height(0);

		if (height > importanceGroupingHeight) {
			effectiveHeight = height - importanceGroupingHeight;
		}

		auto referenceSnapshot = ReferenceSnapshot<ProperEffectiveBalanceSorting>(effectiveHeight);
		/// If there is no reference snapshot. We return balance as there was never an interaction, or it was removed due to previous calculation approach.
		if (!referenceSnapshot) {
			return GetTrackedBalance();
		}
		/// If there is a reference snapshot then that is our effective balance.
		return referenceSnapshot->GetCompoundEffectiveAmount();
	}

	// Instantiate available functions;
	template Amount AccountBalances::getEffectiveBalance<true>(const Height& height, const uint64_t& importanceGrouping) const;
	template Amount AccountBalances::getEffectiveBalance<false>(const Height& height, const uint64_t& importanceGrouping) const;
	template model::BalancePair AccountBalances::getCompoundEffectiveBalance<true>(const Height& height, const uint64_t& importanceGrouping) const;
	template model::BalancePair AccountBalances::getCompoundEffectiveBalance<false>(const Height& height, const uint64_t& importanceGrouping) const;

	void AccountBalances::pushSnapshot(const model::BalanceSnapshot& snapshot, bool committed) {
		if (!m_accountState)
			CATAPULT_THROW_RUNTIME_ERROR("each balance must have own account");

		if (snapshot.BalanceHeight + Height(1) < m_accountState->AddressHeight)
			CATAPULT_THROW_RUNTIME_ERROR_2(
					"height of snapshot can't be lower than height of account", snapshot.BalanceHeight, m_accountState->AddressHeight);

		if (committed) {
			m_localSnapshots.push_back(snapshot);
		} else {
			m_remoteSnapshots.push_back(snapshot);
		}
	}

	const utils::SimpleMutationTracker<AccountBalancesTrackables>& AccountBalances::changes() const{
		return m_changesTracker;
	}

	void AccountBalances::clearChanges() {
		m_changesTracker.clear();
	}
	StableBalanceSnapshot::StableBalanceSnapshot(
			const uint64_t importanceGrouping,
			const uint32_t maxRollbackBlocks)
		: ImportanceGrouping(importanceGrouping), MaxRollbackBlocks(maxRollbackBlocks) {}

	Height StableBalanceSnapshot::GetUnstableHeight(int multiplier) const {
		return Height((MaxRollbackBlocks * multiplier) + ImportanceGrouping);
	}
}}
