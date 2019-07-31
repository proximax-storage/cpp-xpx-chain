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
#ifndef CUSTOM_RESULT_DEFINITION
#include "plugins/txes/aggregate/src/validators/Results.h"
#include "catapult/validators/ValidationResult.h"

namespace catapult { namespace validators {

#endif
/// Defines a multisig validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_MULTISIG_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, Multisig, DESCRIPTION, CODE, None)

	/// Validation failed because account is specified to be both added and removed.
	DEFINE_MULTISIG_RESULT(Modify_Account_In_Both_Sets, 1);

	/// Validation failed because multiple removals are present.
	DEFINE_MULTISIG_RESULT(Modify_Multiple_Deletes, 2);

	/// Validation failed because redundant modifications are present.
	DEFINE_MULTISIG_RESULT(Modify_Redundant_Modifications, 3);

	/// Validation failed because account is not in multisig cache.
	DEFINE_MULTISIG_RESULT(Modify_Unknown_Multisig_Account, 4);

	/// Validation failed because account to be removed is not present.
	DEFINE_MULTISIG_RESULT(Modify_Not_A_Cosigner, 5);

	/// Validation failed because account to be added is already a cosignatory.
	DEFINE_MULTISIG_RESULT(Modify_Already_A_Cosigner, 6);

	/// Validation failed because new minimum settings are out of range.
	DEFINE_MULTISIG_RESULT(Modify_Min_Setting_Out_Of_Range, 7);

	/// Validation failed because min settings are larger than number of cosignatories.
	DEFINE_MULTISIG_RESULT(Modify_Min_Setting_Larger_Than_Num_Cosignatories, 8);

	/// Validation failed because the modification type is unsupported.
	DEFINE_MULTISIG_RESULT(Modify_Unsupported_Modification_Type, 9);

	/// Validation failed because the cosignatory already cosigns the maximum number of accounts.
	DEFINE_MULTISIG_RESULT(Modify_Max_Cosigned_Accounts, 10);

	/// Validation failed because the multisig account already has the maximum number of cosignatories.
	DEFINE_MULTISIG_RESULT(Modify_Max_Cosigners, 11);

	/// Validation failed because a multisig loop is created.
	DEFINE_MULTISIG_RESULT(Modify_Loop, 12);

	/// Validation failed because the max multisig depth is exceeded.
	DEFINE_MULTISIG_RESULT(Modify_Max_Multisig_Depth, 13);

	/// Validation failed because an operation is not permitted by a multisig account.
	DEFINE_MULTISIG_RESULT(Operation_Not_Permitted_By_Account, 0x0800);

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_MULTISIG_RESULT(Plugin_Config_Malformed, 14);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
