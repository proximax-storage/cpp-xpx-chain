/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_RESULT_DEFINITION
#include "catapult/validators/ValidationResult.h"

namespace catapult { namespace validators {

#endif
/// Defines a committee validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_CATAPULT_COMMITTEE_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, Committee, DESCRIPTION, CODE, None)

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_CATAPULT_COMMITTEE_RESULT(Plugin_Config_Malformed, 1);

	/// Validation failed because the account already registered.
	DEFINE_CATAPULT_COMMITTEE_RESULT(Redundant, 2);

	/// Validation failed because the account is an ineligible harvester.
	DEFINE_CATAPULT_COMMITTEE_RESULT(Harvester_Ineligible, 3);

	/// Validation failed because the account does not exist.
	DEFINE_CATAPULT_COMMITTEE_RESULT(Account_Does_Not_Exist, 4);

	/// Validation failed because the transaction signer is not owner.
	DEFINE_CATAPULT_COMMITTEE_RESULT(Signer_Is_Not_Owner, 5);

	/// Validation failed because the harvester account is already disabled.
	DEFINE_CATAPULT_COMMITTEE_RESULT(Harvester_Already_Disabled, 6);

	/// Validation failed because block signer is invalid.
	DEFINE_CATAPULT_COMMITTEE_RESULT(Invalid_Block_Signer, 7);

	/// Validation failed because committee round is invalid.
	DEFINE_CATAPULT_COMMITTEE_RESULT(Invalid_Committee_Round, 8);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
