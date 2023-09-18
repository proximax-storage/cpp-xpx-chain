/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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

#include "AccountStateCacheUtils.h"
#include "catapult/model/Address.h"

namespace catapult { namespace cache {

	namespace {
		template<typename TAccountStateCache, typename TAccountState>
		void ProcessForwardedAccountStateT(TAccountStateCache& cache, const Key& key, const consumer<TAccountState&>& action) {
			auto accountStateIter = cache.find(key);
			auto& accountState = accountStateIter.get();

			if (state::AccountType::Remote != accountState.AccountType) {
				action(accountState);
				return;
			}

			auto linkedAccountStateIter = cache.find(state::GetLinkedPublicKey(accountState));
			auto& linkedAccountState = linkedAccountStateIter.get();

			// this check is merely a precaution and will only fire if there is a bug that has corrupted links
			RequireLinkedRemoteAndMainAccounts(accountState, linkedAccountState);
			action(linkedAccountState);
		}
	}

	void ProcessForwardedAccountState(
			AccountStateCacheDelta& cache,
			const Key& key,
			const consumer<state::AccountState&>& action) {
		ProcessForwardedAccountStateT(cache, key, action);
	}

	void ProcessForwardedAccountState(
			const ReadOnlyAccountStateCache& cache,
			const Key& key,
			const consumer<const state::AccountState&>& action) {
		ProcessForwardedAccountStateT(cache, key, action);
	}

	std::unique_ptr<const state::AccountState> FindAccountStateByPublicKeyOrAddress(const cache::ReadOnlyAccountStateCache& cache, const Key& publicKey) {
		auto accountStateKeyIter = cache.find(publicKey);
		if (accountStateKeyIter.tryGet())
			return std::make_unique<const state::AccountState>(accountStateKeyIter.get());

		// if state could not be accessed by public key, try searching by address
		auto accountStateAddressIter = cache.find(model::PublicKeyToAddress(publicKey, cache.networkIdentifier()));
		if (accountStateAddressIter.tryGet())
			return std::make_unique<const state::AccountState>(accountStateAddressIter.get());

		return nullptr;
	}

	state::AccountState* GetAccountStateByPublicKeyOrAddress(cache::AccountStateCacheDelta& cache, const Key& publicKey) {
		auto accountStateKeyIter = cache.find(publicKey);
		if (accountStateKeyIter.tryGet())
			return &accountStateKeyIter.get();

		// if state could not be accessed by public key, try searching by address
		auto accountStateAddressIter = cache.find(model::PublicKeyToAddress(publicKey, cache.networkIdentifier()));
		if (accountStateAddressIter.tryGet())
			return &accountStateAddressIter.get();

		return nullptr;
	}

	const Key GetCurrentlyActiveAccountKey(const cache::ReadOnlyAccountStateCache& cache, const state::AccountState& state) {
		auto trackedState = state;
		while(trackedState.SupplementalPublicKeys.upgrade()){
			trackedState = cache.find(trackedState.SupplementalPublicKeys.upgrade().get()).get();
		}
		return trackedState.PublicKey;
	}

	const Key GetCurrentlyActiveAccountKey(const cache::ReadOnlyAccountStateCache& cache, const Key& accountKey) {
		auto trackedState = cache.find(accountKey).get();
		while(trackedState.SupplementalPublicKeys.upgrade()){
			trackedState = cache.find(trackedState.SupplementalPublicKeys.upgrade().get()).get();
		}
		return trackedState.PublicKey;
	}
}}
