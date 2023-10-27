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
#include "VersionableState.h"
#include "catapult/constants.h"
#include "AccountPublicKeys.h"
#include <optional>
#include <bitset>

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
		Remote_Unlinked,

		/// Account has been locked. Locked accounts can no longer change or participate in the blockchain.
		Locked
	};

	/// Relation of accounts, as upgraded from or to.
	enum class UpgradeRelationTypeResult : uint8_t {
		/// Accounts are unrelated.
		Unrelated,

		/// Account has been upgraded from specified account.
		UpgradeFrom = 0x01,

		/// Account was upgraded to specified account..
		UpgradedTo = 0x02
	};

	MAKE_BITWISE_ENUM(UpgradeRelationTypeResult)

	enum class AdditionalDataFlags : size_t {
		HasOldState = 0
	};

	/// Returns \c true if \a account has been upgraded or is the result of an upgrade.
	UpgradeRelationTypeResult IsUpgraded(const AccountState& accountState);

	/// Account state data.
	struct AccountState : public VersionableState {

	public:

		/// Creates an account state from an \a address and a height (\a addressHeight) for a given version while specifying the previous state
		explicit AccountState(const catapult::Address& address, Height addressHeight, uint32_t version, const AccountState& oldState)
				: Address(address)
				, AddressHeight(addressHeight)
				, PublicKey()
				, PublicKeyHeight(0)
				, AccountType(AccountType::Unlinked)
				, Balances(this)
				, OldState(std::make_shared<AccountState>(oldState))
				, VersionableState(version)
		{}

		/// Creates an account state from an \a address and a height (\a addressHeight) for a given version.
		explicit AccountState(const catapult::Address& address, Height addressHeight, uint32_t version)
				: Address(address)
				, AddressHeight(addressHeight)
				, PublicKey()
				, PublicKeyHeight(0)
				, AccountType(AccountType::Unlinked)
				, Balances(this)
				, OldState(nullptr)
				, VersionableState(version)
		{}

		/// Creates an account state from an \a address and a height (\a addressHeight) with version 1.
		explicit AccountState(const catapult::Address& address, Height addressHeight)
				: Address(address)
				, AddressHeight(addressHeight)
				, PublicKey()
				, PublicKeyHeight(0)
				, AccountType(AccountType::Unlinked)
				, Balances(this)
				, OldState(nullptr)
				, VersionableState(1)
		{}

		/// Copy constructor that makes a deep copy of \a accountState.
		AccountState(const AccountState& accountState)
				: Balances(this)
				, OldState(nullptr)
				, VersionableState(accountState) {
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

		/// Supplemental public keys.
		AccountPublicKeys SupplementalPublicKeys;

		/// Balances of an account.
		AccountBalances Balances;

		/// Account state snapshot prior to upgrade
		std::shared_ptr<state::AccountState> OldState;

	public:
		bool IsLocked() const
		{
			return AccountType == state::AccountType::Locked;
		}

		auto GetAdditionalDataMask() const
		{
			uint8_t data = 0;
			data |= !!OldState << static_cast<uint8_t>(AdditionalDataFlags::HasOldState);
			return data;
		}
	};

	bool HasAdditionalData(AdditionalDataFlags flag, uint8_t mask);

	/// Returns \c true if \a accountType corresponds to a remote account.
	bool IsRemote(AccountType accountType);

	/// Gets the linked public key associated with \a accountState or a zero key.
	Key GetLinkedPublicKey(const AccountState& accountState);

	/// Gets the node public key associated with \a accountState or a zero key.
	Key GetNodePublicKey(const AccountState& accountState);

	/// Gets the vrf public key associated with \a accountState or a zero key.
	Key GetVrfPublicKey(const AccountState& accountState);

	/// Gets the upgrade public key associated with \a accountState or a zero key.
	Key GetUpgradePublicKey(const AccountState& accountState);

	/// Gets the public key of the account this \a accountState was upgraded from or a zero key.
	Key GetPreviousPublicKey(const AccountState& accountState);

	/// Requires that \a remoteAccountState and \a mainAccountState state are linked.
	void RequireLinkedRemoteAndMainAccounts(const AccountState& remoteAccountState, const AccountState& mainAccountState);
}}
