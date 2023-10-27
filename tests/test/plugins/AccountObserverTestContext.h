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
#include "ObserverTestContext.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace test {

	constexpr MosaicId Test_Mosaic = MosaicId(12345);

	/// Observer test context that wraps an observer context and exposes functions for interacting with the account state cache.
	class AccountObserverTestContext : public test::ObserverTestContext {
	public:
		using ObserverTestContext::ObserverTestContext;

	public:
		/// Finds the account identified by \a address.
		const std::unique_ptr<state::AccountState> find(const Address& address) const {
			auto iterator = cache().sub<cache::AccountStateCache>().find(address);
			return std::make_unique<state::AccountState>(*iterator.tryGet());
		}

		/// Finds the account identified by \a publicKey.
		const std::unique_ptr<state::AccountState> find(const Key& publicKey) const {
			auto iterator = cache().sub<cache::AccountStateCache>().find(publicKey);
			return std::make_unique<state::AccountState>(*iterator.tryGet());
		}

	private:
		state::AccountState& addAccount(const Address& address) {
			auto& accountStateCache = cache().sub<cache::AccountStateCache>();
			accountStateCache.addAccount(address, Height(1234));
			return accountStateCache.find(address).get();
		}

		state::AccountState& addAccount(const Key& publicKey) {
			auto& accountStateCache = cache().sub<cache::AccountStateCache>();
			accountStateCache.addAccount(publicKey, Height(1));
			return accountStateCache.find(publicKey).get();
		}

	public:
		/// Sets the (xem) balance of the account identified by \a accountIdentifier to \a amount.
		template<typename IdType>
		state::AccountBalances& setAccountBalance(const IdType& accountIdentifier, Amount::ValueType amount, const Height& height) {
			return setAccountBalance(accountIdentifier, Amount(amount), height);
		}

		/// Sets the (xem) balance of the account identified by \a accountIdentifier to \a amount.
		template<typename IdType>
		state::AccountBalances& setAccountBalance(const IdType& accountIdentifier, Amount amount, const Height& height) {
			auto& accountState = addAccount(accountIdentifier);
			accountState.Balances.credit(Test_Mosaic, amount, height);
			accountState.Balances.track(Test_Mosaic);
			return accountState.Balances;
		}

		/// Gets the (xem) balance of the account identified by \a accountIdentifier.
		template<typename IdType>
		Amount getAccountBalance(const IdType& accountIdentifier) const {
			auto pAccountState = find(accountIdentifier);
			if (!pAccountState)
			CATAPULT_THROW_RUNTIME_ERROR_1("could not find account in cache", utils::HexFormat(accountIdentifier));

			return pAccountState->Balances.get(Test_Mosaic);
		}
	};
}}
