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
#include "catapult/constants.h"

namespace catapult { namespace state {

	namespace {
		constexpr static bool IsZero(Amount amount) {
			return Amount(0) == amount;
		}

		inline auto minSnapshot(const std::deque<model::BalanceSnapshot>& snapshots, const Height& height, const uint64_t& effectiveBalanceRange) {
			auto effectiveBalanceRangeHeight = Height(effectiveBalanceRange);
			auto effectiveHeight = Height(0);

			if (height > effectiveBalanceRangeHeight) {
				effectiveHeight = height - effectiveBalanceRangeHeight;
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
		if (!m_accountState) {
			m_accountState = accountBalances.m_accountState;
		}

		for (const auto& pair : accountBalances)
			m_balances.insert(pair);

		for (const auto& snapshot : accountBalances.m_snapshots)
			pushSnapshot(snapshot);

		return *this;
	}

	AccountBalances& AccountBalances::operator=(AccountBalances&& accountBalances) = default;

	Amount AccountBalances::get(MosaicId mosaicId) const {
		auto iter = m_balances.find(mosaicId);
		return m_balances.end() == iter ? Amount(0) : iter->second;
	}

	AccountBalances& AccountBalances::credit(MosaicId mosaicId, Amount amount, Height height) {
		if (IsZero(amount))
			return *this;

		auto iter = m_balances.find(mosaicId);
		if (m_balances.end() == iter) {
			if (m_snapshots.empty()) {
				maybePushSnapshot(mosaicId, Amount(0), height - Height(1));
			}
			m_balances.insert(std::make_pair(mosaicId, amount));
			maybePushSnapshot(mosaicId, amount, height);
		} else {
			if (m_snapshots.empty()) {
				maybePushSnapshot(mosaicId, iter->second, height - Height(1));
			}
			iter->second = iter->second + amount;
			maybePushSnapshot(mosaicId, iter->second, height);
		}

		return *this;
	}

	AccountBalances& AccountBalances::debit(MosaicId mosaicId, Amount amount, Height height) {
		if (IsZero(amount))
			return *this;

		auto iter = m_balances.find(mosaicId);
		auto hasZeroBalance = m_balances.end() == iter;
		if (hasZeroBalance || amount > iter->second) {
			CATAPULT_THROW_RUNTIME_ERROR_2(
					"debit amount is greater than current balance",
					amount,
					hasZeroBalance ? Amount(0) : iter->second);
		}

		iter->second = iter->second - amount;

		maybePushSnapshot(mosaicId, iter->second, height);

		if (IsZero(iter->second))
			m_balances.erase(mosaicId);

		return *this;
	}

	void AccountBalances::maybeCleanUpSnapshots(const Height& height, const model::BlockChainConfiguration config) {
		auto unstableHeight = Height(config.EffectiveBalanceRange + config.MaxRollbackBlocks);

		if (height <= unstableHeight) {
			return;
		}

		auto stableHeight = height - unstableHeight;

		while(!m_snapshots.empty() && m_snapshots.front().BalanceHeight <= stableHeight) {
			popSnapshot(false /* back */);
		}
	}

	Amount AccountBalances::getEffectiveBalance(const Height& height, const uint64_t& effectiveBalanceRange) const {
		if (m_snapshots.empty()) {
			auto iter = m_balances.find(Xpx_Id);
			return m_balances.end() == iter ? Amount(0) : iter->second;
		}

		return minSnapshot(m_snapshots, height, effectiveBalanceRange)->Amount;
	}

	void AccountBalances::maybePushSnapshot(const MosaicId& mosaicId, const Amount& amount, const Height& height) {
		if (mosaicId != Xpx_Id || height == Height(0) || height == Height(-1)) {
			return;
		}

		if (m_snapshots.empty()) {
			pushSnapshot(model::BalanceSnapshot{amount, height});
			return;
		}

		if (height < m_snapshots.back().BalanceHeight) {
			CATAPULT_THROW_RUNTIME_ERROR_2(
					"height can't be lower than height of snapshot", height, m_snapshots.back().BalanceHeight);
		} else if (height == m_snapshots.back().BalanceHeight) {
			popSnapshot();
			pushSnapshot(model::BalanceSnapshot{amount, height});
		} else if (height > m_snapshots.back().BalanceHeight) {
			pushSnapshot(model::BalanceSnapshot{amount, height});
		}

		while(m_snapshots.size() > 1 && m_snapshots[m_snapshots.size() - 1].Amount == m_snapshots[m_snapshots.size() - 2].Amount) {
			popSnapshot();
		}
	}

	void AccountBalances::pushSnapshot(const model::BalanceSnapshot& snapshot) {
		if (!m_accountState) {
			CATAPULT_THROW_RUNTIME_ERROR("each balance must have own account");
		}

		if (snapshot.BalanceHeight + Height(1) < m_accountState->AddressHeight) {
			CATAPULT_THROW_RUNTIME_ERROR_2(
					"height of snapshot can't be lower than height of account", snapshot.BalanceHeight, m_accountState->AddressHeight);
		}

		m_snapshots.push_back(snapshot);
	}

	void AccountBalances::popSnapshot(bool back) {
		if (back) {
			m_snapshots.pop_back();
		} else {
			m_snapshots.pop_front();
		}
	}
}}
