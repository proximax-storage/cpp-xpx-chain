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
#include "catapult/types.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/extensions/LocalNodeStateRef.h"

namespace catapult { namespace cache { class ReadOnlyAccountStateCache; } }

namespace catapult { namespace cache {

	/// A view on top of an account state cache for retrieving balance.
	class BalanceView {
	public:
		/// Creates a view around \a cache.
		explicit BalanceView(
				ReadOnlyAccountStateCache cache,
				const Height& effectiveBalanceHeight)
				: m_cache(cache)
				, m_effectiveBalanceHeight(effectiveBalanceHeight)
		{}

	public:
		/// Returns \c true if \a publicKey can harvest at \a height, given a minimum harvesting balance of
		/// \a minHarvestingBalance.
		bool canHarvest(const Key& publicKey, Height height, Amount minHarvestingBalance) const;

	private:
		ReadOnlyAccountStateCache m_cache;
		Height m_effectiveBalanceHeight;
	};
}}
