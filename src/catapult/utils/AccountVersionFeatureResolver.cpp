/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/state/AccountState.h"
#include "AccountVersionFeatureResolver.h"
namespace catapult { namespace utils {


	DerivationScheme AccountVersionFeatureResolver::KeyDerivationScheme(uint32_t accountVersion)
	{
#define RANGE ((1)(2))
#define TEMPLATEDACCOUNTTORUNTIMEEVALUATION(r, p) \
if (BOOST_PP_SEQ_ELEM(0, p) == accountVersion) \
	return KeyDerivationScheme<BOOST_PP_TUPLE_REM_CTOR(1, BOOST_PP_SEQ_TO_TUPLE(p))>();
		BOOST_PP_SEQ_FOR_EACH_PRODUCT(TEMPLATEDACCOUNTTORUNTIMEEVALUATION, RANGE)
#undef TEMPLATEDACCOUNTTORUNTIMEEVALUATION
#undef RANGE
	}

	bool AccountVersionFeatureResolver::MinimumVersionHasCompatibleDerivationScheme(DerivationScheme scheme, uint32_t minAccountVersion, uint32_t currentAccountVersion)
	{
		for(size_t val = minAccountVersion; val <= currentAccountVersion; val++)
		{
			if(KeyDerivationScheme(val) == scheme)
			{
				return true;
			}
		}
		return false;
	}

	bool AccountVersionFeatureResolver::IsAccountShemeCompatible(uint32_t accountVersion, DerivationScheme scheme)
	{
		auto accountScheme = KeyDerivationScheme(accountVersion);
		// Second condition exists for backwards compatibility when blocks/transactions did not have the derivation scheme bit set.
		return accountScheme == scheme || (scheme == DerivationScheme::Unset && accountVersion == 1);
	}
}}
