/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/utils/ArraySet.h"
#include "src/state/LiquidityProviderEntry.h"

namespace catapult { namespace cache {

	/// A class that collects LiquidityProvider keys from LiquidityProvider cache entries.
	class LiquidityProviderKeyCollector {
	public:
		/// Adds a key stored in \a entry.
		void addKey(const state::LiquidityProviderEntry& entry) {
			m_keys.insert(entry.mosaicId());
		}

		/// Returns collected keys.
		std::unordered_set<UnresolvedMosaicId, utils::BaseValueHasher<UnresolvedMosaicId>>& keys() {
			return m_keys;
		}

		/// Returns collected keys.
		const std::unordered_set<UnresolvedMosaicId, utils::BaseValueHasher<UnresolvedMosaicId>>& keys() const {
			return m_keys;
		}

	private:
		std::unordered_set<UnresolvedMosaicId, utils::BaseValueHasher<UnresolvedMosaicId>> m_keys;
	};
}}
