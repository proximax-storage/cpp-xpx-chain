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
/// Defines a transfer validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_LockFund_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, LockFund, DESCRIPTION, CODE, None)

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_LockFund_RESULT(Duration_Smaller_Than_Configured, 1);

	/// Validation failed because sender does not meet requirements.
	DEFINE_LockFund_RESULT(Invalid_Sender, 2);

	/// Validation failed because there are not enough funds to lock/unlock.
	DEFINE_LockFund_RESULT(Not_Enough_Funds, 3);

	/// Validation failed because a record already exists at this height for this key.
	DEFINE_LockFund_RESULT(Duplicate_Record, 4);

	/// Validation failed because no record exists at this height for this key.
	DEFINE_LockFund_RESULT(Request_Non_Existant, 5);

	/// Validation failed because mosaics are out of order.
	DEFINE_LockFund_RESULT(Out_Of_Order_Mosaics, 6);

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_LockFund_RESULT(Plugin_Config_Malformed, 7);

	/// Validation failed because number of mosaics exceeded the limit.
	DEFINE_LockFund_RESULT(Too_Many_Mosaics, 8);

	/// Validation failed because mosaic amount is zero.
	DEFINE_LockFund_RESULT(Zero_Amount, 9);

	/// Validation failed because this account already has the maximum amount of unlock requests set.
	DEFINE_LockFund_RESULT(Maximum_Unlock_Records, 10);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
