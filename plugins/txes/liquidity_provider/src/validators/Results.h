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

	/// The liquidity provider is tried to be manager by invalid owner
	DEFINE_LIQUIDITY_PROVIDER_RESULT(Invalid_Owner, 2);

	// The Liquidity Provider Slashing Period Is Invalid
	DEFINE_LIQUIDITY_PROVIDER_RESULT(Invalid_Slashing_Period, 3);

	// The Liquidity Provider History Window Size Is Invalid
	DEFINE_LIQUIDITY_PROVIDER_RESULT(Invalid_Window_Size, 4);

	// The Liquidity Provider Is Not Registered
	DEFINE_LIQUIDITY_PROVIDER_RESULT(Liquidity_Provider_Is_Not_Registered, 5);

	// Insufficient Currency on Balance
	DEFINE_LIQUIDITY_PROVIDER_RESULT(Insufficient_Currency, 6);

	// Insufficient Currency on Balance
	DEFINE_LIQUIDITY_PROVIDER_RESULT(Insufficient_Mosaic, 7);

	// Insufficient Exchange Rate
	DEFINE_LIQUIDITY_PROVIDER_RESULT(Invalid_Exchange_Rate, 8);

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_LIQUIDITY_PROVIDER_RESULT(Plugin_Config_Malformed, 9);


#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
