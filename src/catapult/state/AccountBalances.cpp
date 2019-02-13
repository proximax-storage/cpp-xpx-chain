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

namespace catapult { namespace state {

	namespace {
		constexpr static bool IsZero(Amount amount) {
			return Amount(0) == amount;
		}

		inline auto minSnapshot(const std::list<model::BalanceSnapshot>& snapshots, const Height& height, const uint64_t& importanceGrouping) {
			auto importanceGroupingHeight = Height(importanceGrouping);
			auto effectiveHeight = Height(0);

			if (height > importanceGroupingHeight) {
				effectiveHeight = height - importanceGroupingHeight;
			}

			return std::min_element(
				snapshots.begin(),
				snapshots.end(),
				[&effectiveHeight](const model::BalanceSnapshot& l, const model::BalanceSnapshot& r){
					return l.BalanceHeight <= effectiveHeight || l.Amount < r.Amount;
				}
			);
		}
	}

	AccountBalances::AccountBalances(AccountState* accountState) : m_accountState(accountState) {}

	AccountBalances::AccountBalances(const AccountBalances& accountBalances) {
		*this = accountBalances;
	}

	AccountBalances::AccountBalances(AccountBalances&& accountBalances) = default;

	AccountBalances& AccountBalances::operator=(const AccountBalances& accountBalances) {
		m_optimizedMosaicId = accountBalances.optimizedMosaicId();
		m_trackedMosaicId = accountBalances.trackedMosaicId();
		m_balances.optimize(m_optimizedMosaicId);
		if (!m_accountState) {
			m_accountState = accountBalances.m_accountState;
		}

		for (const auto& pair : accountBalances)
			m_balances.insert(pair);

		for (const auto& snapshot : accountBalances.m_localSnapshots)
			pushSnapshot(snapshot, true /* committed */);

		for (const auto& snapshot : accountBalances.m_remoteSnapshots)
			pushSnapshot(snapshot, false /* committed */);

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

	AccountBalances& AccountBalances::internalCredit(const MosaicId& mosaicId, const Amount& amount, const Height& height) {
		if (IsZero(amount))
			return *this;

		auto iter = m_balances.find(mosaicId);
		if (m_balances.end() == iter) {
			if (m_remoteSnapshots.empty() && m_localSnapshots.empty()) {
				maybePushSnapshot(mosaicId, Amount(0), height - Height(1));
			}
			m_balances.insert(std::make_pair(mosaicId, amount));
			maybePushSnapshot(mosaicId, amount, height);
		} else {
			if (m_remoteSnapshots.empty() && m_localSnapshots.empty()) {
				maybePushSnapshot(mosaicId, iter->second, height - Height(1));
			}
			iter->second = iter->second + amount;
			maybePushSnapshot(mosaicId, iter->second, height);
		}

		return *this;
	}

	AccountBalances& AccountBalances::internalDebit(const MosaicId& mosaicId, const Amount& amount, const Height& height) {
		if (IsZero(amount))
			return *this;

		auto iter = m_balances.find(mosaicId);
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

		maybePushSnapshot(mosaicId, iter->second, height);

		if (IsZero(iter->second))
			m_balances.erase(mosaicId);

		return *this;
	}

	void AccountBalances::optimize(MosaicId id) {
		m_balances.optimize(id);
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
				&& m_localSnapshots.back().Amount == m_remoteSnapshots.front().Amount) {
			m_remoteSnapshots.pop_front();
		}

		m_localSnapshots.splice(m_localSnapshots.end(), m_remoteSnapshots);
	}

	void AccountBalances::maybeCleanUpSnapshots(const Height& height, const model::BlockChainConfiguration& config) {
		auto unstableHeight = Height(config.ImportanceGrouping + config.MaxRollbackBlocks);

		if (height <= unstableHeight) {
			return;
		}

		auto stableHeight = height - unstableHeight;

		while(!m_localSnapshots.empty() && m_localSnapshots.front().BalanceHeight <= stableHeight) {
			m_localSnapshots.pop_front();
		}
	}

	Amount AccountBalances::getEffectiveBalance(const Height& height, const uint64_t& importanceGrouping) const {
		if (m_localSnapshots.empty() && m_remoteSnapshots.empty()) {
			auto iter = m_balances.find(m_trackedMosaicId);
			return m_balances.end() == iter ? Amount(0) : iter->second;
		}

		if (m_remoteSnapshots.empty()) {
			return minSnapshot(m_localSnapshots, height, importanceGrouping)->Amount;
		} else if (m_localSnapshots.empty()) {
			return minSnapshot(m_remoteSnapshots, height, importanceGrouping)->Amount;
		} else {
			return std::min(
				minSnapshot(m_localSnapshots, height, importanceGrouping)->Amount,
				minSnapshot(m_remoteSnapshots, height, importanceGrouping)->Amount
			);
		}
	}

	void AccountBalances::maybePushSnapshot(const MosaicId& mosaicId, const Amount& amount, const Height& height) {
		if (mosaicId != m_trackedMosaicId || height == Height(0) || height == Height(-1)) {
			return;
		}

		if (m_remoteSnapshots.empty()) {
			pushSnapshot(model::BalanceSnapshot{amount, height});
			return;
		}

		if (height <= m_remoteSnapshots.back().BalanceHeight) {
			while(!m_remoteSnapshots.empty() &&  m_remoteSnapshots.back().BalanceHeight >= height) {
				m_remoteSnapshots.pop_back();
			}
			pushSnapshot(model::BalanceSnapshot{amount, height});
		} else if (height > m_remoteSnapshots.back().BalanceHeight) {
			pushSnapshot(model::BalanceSnapshot{amount, height});
		}
	}

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
}}
