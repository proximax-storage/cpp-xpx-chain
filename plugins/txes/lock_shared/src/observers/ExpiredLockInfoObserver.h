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

namespace catapult { namespace observers {

	/// On commit, credits the expiration account of expired locks.
	/// On rollback, debits the expiration account of expired locks.
	/// Uses the observer \a context to determine notification direction and access caches.
	/// Uses \a ownerAccountIdSupplier to retrieve the lock owner's account identifier.
	template<typename TLockInfoCache, typename TAccountIdSupplier>
	void ExpiredLockInfoObserver(const ObserverContext& context, TAccountIdSupplier ownerAccountIdSupplier) {
		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto& lockInfoCache = context.Cache.template sub<TLockInfoCache>();

		lockInfoCache.processUnusedExpiredLocks(context.Height, [&context, &accountStateCache, ownerAccountIdSupplier](
				const auto& lockInfo) {
			
			auto accountStateIter = accountStateCache.find(ownerAccountIdSupplier(lockInfo));
			auto& accountState = accountStateIter.get();
			
			auto applyPayment = [lockInfo, context](auto& account) {
				if (NotifyMode::Commit == context.Mode)
					for (const auto& pair : lockInfo.Mosaics)
						account.Balances.credit(pair.first, pair.second, context.Height);
				else
					for (const auto& pair : lockInfo.Mosaics)
						account.Balances.debit(pair.first, pair.second, context.Height);
			};
			
			switch (lockInfo.Version) {
			
			case 1:
				applyPayment(accountState);
				break;
			
			case 2:
				if (state::AccountType::Remote != accountState.AccountType) {
					applyPayment(accountState);
					return;
				}
				
				auto linkedAccountStateIter = accountStateCache.find(accountState.LinkedAccountKey);
				auto &linkedAccountState = linkedAccountStateIter.get();
				
				// this check is merely a precaution and will only fire if there is a bug that has corrupted links
				RequireLinkedRemoteAndMainAccounts(accountState, linkedAccountState);
					
				applyPayment(linkedAccountState);
				break;
			}
		});
	}
}}
