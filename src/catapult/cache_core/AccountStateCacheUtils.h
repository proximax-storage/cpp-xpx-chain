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

#pragma once
#include "catapult/functions.h"
#include "catapult/types.h"
#include <optional>
namespace catapult {
	namespace cache {
		class AccountStateCacheDelta;
		class ReadOnlyAccountStateCache;
	}
	namespace state { struct AccountState; }
}

namespace catapult { namespace cache {

	/// Forwards account state or linked account state found in \a cache associated with \a address to \a action.
	void ProcessForwardedAccountState(AccountStateCacheDelta& cache, const Key& key, const consumer<state::AccountState&>& action);

	/// Forwards account state or linked account state found in \a cache associated with \a address to \a action.
	void ProcessForwardedAccountState(
			const ReadOnlyAccountStateCache& cache,
			const Key& key,
			const consumer<const state::AccountState&>& action);

	/// Find an account state by public key or address using a ReadOnlyAccountStateCache \a cache and a \a publicKey and a pointer to a constant copy.
	std::unique_ptr<const state::AccountState> FindAccountStateByPublicKeyOrAddress(const cache::ReadOnlyAccountStateCache& cache, const Key& publicKey);
	/// Find an account state by public key or address using a AccountStateCacheDelta \a cache and a \a publicKey. Returns a pointer to the actual data. Note: AccountState not owned
	state::AccountState* GetAccountStateByPublicKeyOrAddress(cache::AccountStateCacheDelta& cache, const Key& publicKey);
}}
