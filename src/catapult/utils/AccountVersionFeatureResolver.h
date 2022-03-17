/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
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
