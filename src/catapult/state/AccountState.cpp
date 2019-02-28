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

#include "AccountState.h"
#include "catapult/model/Address.h"

namespace catapult { namespace state {

	namespace {
		void RequireAccountType(const AccountState& accountState, AccountType requiredAccountType, const char* requiredAccountTypeName) {
			if (requiredAccountType == accountState.AccountType)
				return;

			std::ostringstream out;
			out << "expected " << model::AddressToString(accountState.Address) << " to have account type " << requiredAccountTypeName;
			CATAPULT_THROW_RUNTIME_ERROR(out.str().c_str());
		}

		bool AreLinked(const AccountState& lhs, const AccountState& rhs) {
			return lhs.LinkedAccountKey == rhs.PublicKey && rhs.LinkedAccountKey == lhs.PublicKey;
		}
	}

	bool IsRemote(AccountType accountType) {
		switch (accountType) {
		case AccountType::Remote:
		case AccountType::Remote_Unlinked:
			return true;

		default:
			return false;
		}
	}

	void RequireLinkedRemoteAndMainAccounts(const AccountState& remoteAccountState, const AccountState& mainAccountState) {
		RequireAccountType(remoteAccountState, AccountType::Remote, "REMOTE");
		RequireAccountType(mainAccountState, AccountType::Main, "MAIN");

		if (AreLinked(mainAccountState, remoteAccountState))
			return;

		std::ostringstream out;
		out
				<< "remote " << model::AddressToString(remoteAccountState.Address) << " link to main"
				<< model::AddressToString(mainAccountState.Address) << " is improper";
		CATAPULT_THROW_RUNTIME_ERROR(out.str().c_str());
	}
}}
