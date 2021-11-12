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
		return KeyDerivationScheme(accountVersion) == scheme;
	}
}}
