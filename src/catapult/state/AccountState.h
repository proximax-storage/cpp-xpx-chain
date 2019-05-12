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
#include "AccountBalances.h"
#include "AccountImportance.h"
#include "catapult/constants.h"
#include "CacheDataEntry.h"

namespace catapult { namespace state {

	/// Possible account types.
	enum class AccountType : uint8_t {
		/// Account is not linked to another account.
		Unlinked,

		/// Account is a balance-holding account that is linked to a remote harvester account.
		Main,

		/// Account is a remote harvester account that is linked to a balance-holding account.
		Remote,

		/// Account is a remote harvester eligible account that is unlinked.
		Remote_Unlinked
	};

	/// Account state data.
	struct AccountState : public CacheDataEntry<AccountState> {
	public:
        static constexpr VersionType MaxVersion{1};

	public:
		/// Creates an account state from an \a address and a height (\a addressHeight).
		explicit AccountState(const catapult::Address& address, Height addressHeight, VersionType version = 1)
				: CacheDataEntry(version)
				, Address(address)
				, AddressHeight(addressHeight)
				, PublicKey()
				, PublicKeyHeight(0)
				, AccountType(AccountType::Unlinked)
				, LinkedAccountKey()
				, Balances(this)
        {}

		/// Copy constructor that makes a deep copy of \a accountState.
		AccountState(const AccountState& accountState)
                : CacheDataEntry(accountState.getVersion())
                , Balances(this) {
			*this = accountState;
		}

	public:
		/// Address of an account.
		catapult::Address Address;

		/// Height at which address has been obtained.
		Height AddressHeight;

		/// Public key of an account. Present if PublicKeyHeight > 0.
		Key PublicKey;

		/// Height at which public key has been obtained.
		Height PublicKeyHeight;

		/// Type of account.
		state::AccountType AccountType;

		/// Public key of linked account.
		Key LinkedAccountKey;

		/// Balances of an account.
		AccountBalances Balances;
	};

	/// Returns \c true if \a accountType corresponds to a remote account.
	bool IsRemote(AccountType accountType);

	/// Requires that \a remoteAccountState and \a mainAccountState state are linked.
	void RequireLinkedRemoteAndMainAccounts(const AccountState& remoteAccountState, const AccountState& mainAccountState);
}}
