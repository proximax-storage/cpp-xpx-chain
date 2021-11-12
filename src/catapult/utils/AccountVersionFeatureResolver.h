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
#include "catapult/state/AccountState.h"
#include "catapult/types.h"

#include <boost/preprocessor.hpp>


namespace catapult { namespace utils {


	class AccountVersionFeatureResolver
	{
	public:
		AccountVersionFeatureResolver() = default;

	public:
		/// Returns the derivation scheme used in this account version
		template<uint32_t TVersion>
		static constexpr DerivationScheme KeyDerivationScheme()
		{
			if constexpr (TVersion == 1)
				return DerivationScheme::Ed25519_Sha3;
			if constexpr (TVersion == 2)
				return DerivationScheme::Ed25519_Sha2;
		}

		template<DerivationScheme TDerivationScheme, uint32_t TAccountVersion>
		static constexpr bool IsAccountShemeCompatible()
		{
			return utils::AccountVersionFeatureResolver::KeyDerivationScheme<TAccountVersion>() == KeyDerivationScheme<TDerivationScheme>();
		}

		/// Gets the key derivation scheme used in a given \a accountVersion
		static DerivationScheme KeyDerivationScheme(uint32_t accountVersion);

		/// Determines if there is an account version that currently supports creation, that uses the given \a scheme
		static bool MinimumVersionHasCompatibleDerivationScheme(DerivationScheme scheme, uint32_t currentAccountVersion, uint32_t minAccountVersion);

		/// Determines if \a accountVersion uses the given \a scheme
		static bool IsAccountShemeCompatible(uint32_t accountVersion, DerivationScheme scheme);
	};


}}
