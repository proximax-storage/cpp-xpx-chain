/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_RESULT_DEFINITION
#include "catapult/validators/ValidationResult.h"

namespace catapult { namespace validators {

#endif
/// Defines a blockchain upgrade validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_CATAPULT_EXCHANGE_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, Exchange, DESCRIPTION, CODE, None)

	/// Validation failed because offer doesn't exist.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Offer_Doesnt_Exist, 1);

	/// Validation failed because offer signer is invalid.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Offer_Signer_Invalid, 2);

	/// Validation failed because unit amount is zero.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Zero_Amount, 3);

	/// Validation failed because unit price is zero.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Zero_Price, 4);

	/// Validation failed because there is no offers.
	DEFINE_CATAPULT_EXCHANGE_RESULT(No_Offers, 5);

	/// Validation failed because mosaic for exchange is invalid.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Invalid_Mosaic, 6);

	/// Validation failed because offer has already been removed.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Offer_Already_Removed, 7);

	/// Validation failed because to buying own units is not allowed.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Buying_Own_Units_Is_Not_Allowed, 8);

	/// Validation failed because unit is not found in offer.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Unit_Not_Found_In_Offer, 9);

	/// Validation failed because there is not enough units in offer.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Not_Enough_Units_In_Offer, 10);

	/// Validation failed because price is invalid in offer.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Invalid_Price, 11);

	/// Validation failed because matched offer type is the same as the type of the offer itself.
	/// Buy offers shouldn't match with buy offers, as well as sell offers shouldn't match with sell offers.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Invalid_Mathed_Offer_Type, 12);

	/// Validation failed because offer expired.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Offer_Expired, 13);

	/// Validation failed because offer duration exceeded maximum.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Offer_Duration_Too_Large, 14);

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Plugin_Config_Malformed, 15);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
