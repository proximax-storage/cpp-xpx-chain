/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/utils/Hashers.h"
#include "src/state/CommitteeEntry.h"
#include <unordered_map>

namespace catapult { namespace cache {

	using AccountMap = std::unordered_map<Key, state::AccountData, utils::ArrayHasher<Key>>;

	/// A class that collects harvester accounts from committee cache entries.
	class CommitteeAccountCollector {
	public:
		/// Adds an account stored in \a entry.
		void addAccount(const state::CommitteeEntry& entry) {
			m_accounts.insert_or_assign(entry.key(), entry.data());
		}

		/// Removes account with \a key.
		void removeAccount(const Key& key) {
			m_accounts.erase(key);
		}

		/// Returns collected accounts.
		AccountMap& accounts() {
			return m_accounts;
		}

		/// Returns collected accounts.
		const AccountMap& accounts() const {
			return m_accounts;
		}

	private:
		AccountMap m_accounts;
	};
}}
