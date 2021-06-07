/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/utils/ArraySet.h"
#include "src/state/ReplicatorEntry.h"

namespace catapult { namespace cache {

	/// A class that collects replicator keys from replicator cache entries.
	class ReplicatorKeyCollector {
	public:
		/// Adds a key stored in \a entry.
		void addKey(const state::ReplicatorEntry& entry) {
			m_keys.insert(entry.key());
		}

		/// Returns collected keys.
		utils::KeySet& keys() {
			return m_keys;
		}

		/// Returns collected keys.
		const utils::KeySet& keys() const {
			return m_keys;
		}

	private:
		utils::KeySet m_keys;
	};
}}
