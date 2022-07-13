/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_RESULT_DEFINITION
#include "catapult/validators/ValidationResult.h"

namespace catapult { namespace validators {

#endif
/// Defines a SDA-SDA exchange validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_CATAPULT_EXCHANGESDA_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, ExchangeSda, DESCRIPTION, CODE, None)

	/// Validation failed because there is no offers.
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(No_Offers, 1);

	/// Validation failed because exchanging own units is not allowed.
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(Exchanging_Own_Units_Is_Not_Allowed, 2);

	/// Validation failed because the account doesn't have any offer.
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(Account_Doesnt_Have_Any_Offer, 3);

	/// Validation failed because there is at least two offers of the same
	/// type with the same mosaic from the same owner.
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(Duplicated_Offer_In_Request, 4);

	/// Validation failed because offer duration is zero.
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(Zero_Offer_Duration, 5);

	/// Validation failed because offer duration exceeded maximum.
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(Offer_Duration_Too_Large, 6);

	/// Validation failed because offer duration exceeds mosaic duration
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(Offer_Duration_Exceeds_Mosaic_Duration, 7);

	/// Validation failed because unit amount is zero.
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(Zero_Amount, 8);

	/// Validation failed because unit price is zero.
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(Zero_Price, 9);

	/// Validation failed because offer already exists.
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(Offer_Exists, 10);

	/// Validation failed because offer doesn't exist.
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(Offer_Doesnt_Exist, 11);

	/// Validation failed because offer expired.
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(Offer_Expired, 12);

	/// Validation failed because price is invalid in offer.
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(Invalid_Price, 13);

	/// Validation failed because there is not enough units in offer.
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(Not_Enough_Units_In_Offer, 14);

	/// Validation failed because there is already removed offer at the height.
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(Cant_Remove_Offer_At_Height, 15);

	/// Validation failed because there is no offered mosaic to remove.
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(No_Offered_Mosaics_To_Remove, 16);

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(Plugin_Config_Malformed, 17);

	/// Validation failed because exchanging same units is not allowed.
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(Exchanging_Same_Units_Is_Not_Allowed, 18);

 	/// Validation failed because mosaic is not found.
	DEFINE_CATAPULT_EXCHANGESDA_RESULT(Mosaic_Not_Found, 19);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif