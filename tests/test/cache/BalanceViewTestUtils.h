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
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/BalanceView.h"

namespace catapult { namespace test {

	/// Wrapper for an importance view.
	class BalanceViewWrapper {
	public:
		/// Creates a wrapper around \a cache.
		explicit BalanceViewWrapper(
				const cache::AccountStateCache& currentCache,
				const cache::AccountStateCache& previousCache,
				const Height& effectiveBalanceHeight)
			: m_currentCacheView(currentCache.createView())
			, m_previousCacheView(previousCache.createView())
			, m_readOnlyCurrentCache(*m_currentCacheView)
			, m_readOnlyPreviousCache(*m_previousCacheView)
			, m_view(m_readOnlyCurrentCache, m_readOnlyPreviousCache, effectiveBalanceHeight)
		{}

	public:
		/// Gets a const reference to the underlying importance view.
		const cache::BalanceView& operator*() {
			return m_view;
		}

		/// Gets a const pointer to the underlying importance view.
		const cache::BalanceView* operator->() {
			return &m_view;
		}

	private:
		cache::LockedCacheView<cache::AccountStateCacheView> m_currentCacheView;
		cache::LockedCacheView<cache::AccountStateCacheView> m_previousCacheView;
		cache::ReadOnlyAccountStateCache m_readOnlyCurrentCache;
		cache::ReadOnlyAccountStateCache m_readOnlyPreviousCache;
		cache::BalanceView m_view;
	};

	/// Creates an balance view wrapper around \a cache.
	CATAPULT_INLINE
	BalanceViewWrapper CreateBalanceView(
			const cache::AccountStateCache& currentCache,
			const cache::AccountStateCache& previousCache,
			const Height& effectiveBalanceHeight) {
		return BalanceViewWrapper(currentCache, previousCache, effectiveBalanceHeight);
	}
}}
