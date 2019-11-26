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

	/// Validation failed because unit amount is zero.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Zero_Amount, 2);

	/// Validation failed because unit price is zero.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Zero_Price, 3);

	/// Validation failed because there is no offers.
	DEFINE_CATAPULT_EXCHANGE_RESULT(No_Offers, 4);

	/// Validation failed because mosaic for exchange is not allowed.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Mosaic_Not_Allowed, 5);

	/// Validation failed because to buying own units is not allowed.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Buying_Own_Units_Is_Not_Allowed, 6);

	/// Validation failed because there is not enough units in offer.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Not_Enough_Units_In_Offer, 7);

	/// Validation failed because price is invalid in offer.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Invalid_Price, 8);

		/// Validation failed because the account doesn't have any offer.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Account_Doesnt_Have_Any_Offer, 9);

	/// Validation failed because offer duration exceeded maximum.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Offer_Duration_Too_Large, 10);

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Plugin_Config_Malformed, 11);

	/// Validation failed because there is no offered mosaic to remove.
	DEFINE_CATAPULT_EXCHANGE_RESULT(No_Offered_Mosaics_To_Remove, 12);

	/// Validation failed because there is at least two offers of the same
	/// type with the same mosaic from the same owner.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Duplicated_Offer_In_Request, 13);

	/// Validation failed because offer already exists.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Offer_Exists, 14);

	/// Validation failed offer duration is zero.
	DEFINE_CATAPULT_EXCHANGE_RESULT(Zero_Offer_Duration, 15);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
