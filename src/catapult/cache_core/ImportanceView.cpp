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

#include "ImportanceView.h"
#include "AccountStateCache.h"
#include "catapult/model/Address.h"
#include "AccountStateCacheUtils.h"

namespace catapult { namespace cache {

	namespace {
		template<typename TAction>
		bool ForwardIfAccountHasImportanceAtHeight(
				const state::AccountState& accountState,
				const ReadOnlyAccountStateCache& cache,
				Height height,
				TAction action) {
			if (state::AccountType::Remote == accountState.AccountType) {
				auto linkedAccountStateIter = cache.find(GetLinkedPublicKey(accountState));
				const auto& linkedAccountState = linkedAccountStateIter.get();

				// this check is merely a precaution and will only fire if there is a bug that has corrupted links
				RequireLinkedRemoteAndMainAccounts(accountState, linkedAccountState);

				return ForwardIfAccountHasImportanceAtHeight(linkedAccountState, cache, height, action);
			}
			if(accountState.IsLocked()) //Locked accounts cannot harvest
				return false;
			return action(accountState);
		}

		template<typename TAction>
		bool FindAccountStateWithImportance(const ReadOnlyAccountStateCache& cache, const Key& publicKey, Height height, TAction action) {
			auto accountStateOpt = cache::FindAccountStateByPublicKeyOrAddress(cache, publicKey);
			if (accountStateOpt) {
				return ForwardIfAccountHasImportanceAtHeight(*accountStateOpt, cache, height, action);
			}
			return false;
		}
	}

	bool ImportanceView::tryGetAccountImportance(const Key& publicKey, Height height, Importance& importance, bool properEffectiveBalanceCalculation) const {
		return FindAccountStateWithImportance(m_cache, publicKey, height, [&](const auto& accountState) {
			if(properEffectiveBalanceCalculation)
				importance = Importance(accountState.Balances.template getEffectiveBalance<true>(height, m_cache.importanceGrouping()).unwrap());
			else
				importance = Importance(accountState.Balances.template getEffectiveBalance<false>(height, m_cache.importanceGrouping()).unwrap());
			return true;
		});
	}

	Importance ImportanceView::getAccountImportanceOrDefault(const Key& publicKey, Height height, bool properEffectiveBalanceCalculation) const {
		Importance importance;
		return tryGetAccountImportance(publicKey, height, importance, properEffectiveBalanceCalculation) ? importance : Importance(0);
	}

	bool ImportanceView::canHarvest(const Key& publicKey, Height height, Amount minHarvestingBalance, Amount maxHarvestingBalance, bool ProperEffectiveBalanceCalculation) const {
		return FindAccountStateWithImportance(m_cache, publicKey, height, [&](const auto& accountState) {
			Amount balance;
			if(ProperEffectiveBalanceCalculation)
				balance = accountState.Balances.template getEffectiveBalance<true>(height, m_cache.importanceGrouping());
			else
				balance = accountState.Balances.template getEffectiveBalance<false>(height, m_cache.importanceGrouping());

		return balance >= minHarvestingBalance && balance <= maxHarvestingBalance;
		});
	}
}}
