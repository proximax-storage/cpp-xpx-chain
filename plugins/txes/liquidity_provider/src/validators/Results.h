/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_RESULT_DEFINITION
#include "catapult/validators/ValidationResult.h"

namespace catapult { namespace validators {

#endif
/// Defines a storage validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_LIQUIDITY_PROVIDER_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, LiquidityProvider, DESCRIPTION, CODE, None)

	/// The liquidity provider has already been created for the corresponding mosaic
	DEFINE_LIQUIDITY_PROVIDER_RESULT(Liquidity_Provider_Already_Exists, 1);

	/// The liquidity provider has already been created for the corresponding mosaic
	DEFINE_LIQUIDITY_PROVIDER_RESULT(Invalid_Owner, 2);

	// The Liquidity Provider Slashing Period Is Invalid
	DEFINE_LIQUIDITY_PROVIDER_RESULT(Invalid_Slashing_Period, 3);

	// The Liquidity Provider History Window Size Is Invalid
	DEFINE_LIQUIDITY_PROVIDER_RESULT(Invalid_Window_Size, 4);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
