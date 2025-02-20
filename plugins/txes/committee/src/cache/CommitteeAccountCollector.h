/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/utils/Hashers.h"
#include "plugins/txes/committee/src/state/CommitteeEntry.h"
#include "plugins/txes/committee/src/config/CommitteeConfiguration.h"
#include <unordered_map>
#include <map>

namespace catapult { namespace cache {

	using AccountMap = std::unordered_map<Key, state::AccountData, utils::ArrayHasher<Key>>;
	using DisabledAccountMap = std::map<Height, std::vector<Key>>;

	/// A class that collects harvester accounts from committee cache entries.
	class CommitteeAccountCollector {
	public:
		/// Adds an account stored in \a entry.
		void addAccount(const state::CommitteeEntry& entry, const config::CommitteeConfiguration& config) {
			auto disabledHeight = entry.disabledHeight();
			if (disabledHeight != Height(0)) {
				m_disabledAccounts[disabledHeight].push_back(entry.key());
			} else {
				auto [iter, success] = m_accounts.insert_or_assign(entry.key(), entry.data());
				if (!success)
					CATAPULT_THROW_RUNTIME_ERROR("Committee account collector failed to add an account");

				if (entry.version() < 3 || !iter->second.FeeInterestDenominator) {
					iter->second.Activity = config.InitialActivityInt;
					iter->second.FeeInterest = config.MinGreedFeeInterest;
					iter->second.FeeInterestDenominator = config.MinGreedFeeInterestDenominator;
				}
			}
		}

		/// Returns collected accounts.
		AccountMap accounts() {
			return m_accounts;
		}

		/// Returns collected disabled accounts.
		DisabledAccountMap disabledAccounts() {
			return m_disabledAccounts;
		}

		void clear() {
			m_accounts.clear();
			m_disabledAccounts.clear();
		}

	private:
		AccountMap m_accounts;
		DisabledAccountMap m_disabledAccounts;
	};
}}
