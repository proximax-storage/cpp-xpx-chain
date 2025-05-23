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

namespace catapult { namespace cache {

	namespace {
		template<typename TAction>
		bool ForwardIfAccountHasImportanceAtHeight(
				const state::AccountState& accountState,
				const ReadOnlyAccountStateCache& cache,
				Height height,
				TAction action) {
			if (state::AccountType::Remote == accountState.AccountType) {
				auto linkedAccountStateIter = cache.find(accountState.LinkedAccountKey);
				const auto& linkedAccountState = linkedAccountStateIter.get();

				// this check is merely a precaution and will only fire if there is a bug that has corrupted links
				RequireLinkedRemoteAndMainAccounts(accountState, linkedAccountState);

				return ForwardIfAccountHasImportanceAtHeight(linkedAccountState, cache, height, action);
			}

			return action(accountState);
		}

		template<typename TAction>
		bool FindAccountStateWithImportance(const ReadOnlyAccountStateCache& cache, const Key& publicKey, Height height, TAction action) {
			auto accountStateKeyIter = cache.find(publicKey);
			if (accountStateKeyIter.tryGet())
				return ForwardIfAccountHasImportanceAtHeight(accountStateKeyIter.get(), cache, height, action);

			// if state could not be accessed by public key, try searching by address
			auto accountStateAddressIter = cache.find(model::PublicKeyToAddress(publicKey, cache.networkIdentifier()));
			if (accountStateAddressIter.tryGet())
				return ForwardIfAccountHasImportanceAtHeight(accountStateAddressIter.get(), cache, height, action);

			return false;
		}
	}

	bool ImportanceView::tryGetAccountImportance(const Key& publicKey, Height height, Importance& importance) const {
		return FindAccountStateWithImportance(m_cache, publicKey, height, [&](const auto& accountState) {
			importance = Importance(accountState.Balances.getEffectiveBalance(height, m_cache.importanceGrouping()).unwrap());
			return true;
		});
	}

	Importance ImportanceView::getAccountImportanceOrDefault(const Key& publicKey, Height height) const {
		Importance importance;
		return tryGetAccountImportance(publicKey, height, importance) ? importance : Importance(0);
	}

	bool ImportanceView::canHarvest(const Key& publicKey, Height height, Amount minHarvestingBalance) const {
		return FindAccountStateWithImportance(m_cache, publicKey, height, [&](const auto& accountState) {
			return accountState.Balances.getEffectiveBalance(height, m_cache.importanceGrouping()) >= minHarvestingBalance;
		});
	}
}}
