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
/// Defines a super contract validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_SUPERCONTRACT_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, SuperContract, DESCRIPTION, CODE, None)

	/// Super contract already exists.
	DEFINE_SUPERCONTRACT_RESULT(Super_Contract_Already_Exists, 1);

	/// Creation of super contract is not permitted.
	DEFINE_SUPERCONTRACT_RESULT(Operation_Is_Not_Permitted, 2);

	/// File related to super contract is not exist.
	DEFINE_SUPERCONTRACT_RESULT(File_Does_Not_Exist, 3);

	/// Someone try to remove file of super contract.
	DEFINE_SUPERCONTRACT_RESULT(Remove_Super_Contract_File, 4);

	/// Drive already ended, so you can create super contract on this drive.
	DEFINE_SUPERCONTRACT_RESULT(Drive_Has_Ended, 5);

	/// Super contract doesn't exist.
	DEFINE_SUPERCONTRACT_RESULT(SuperContract_Does_Not_Exist, 6);

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_SUPERCONTRACT_RESULT(Plugin_Config_Malformed, 7);

	/// Validation failed because end execute transaction is not the last sub transaction.
	DEFINE_SUPERCONTRACT_RESULT(End_Execute_Transaction_Misplaced, 8);

	/// Validation failed because operation identify transaction is not the first sub transaction.
	DEFINE_SUPERCONTRACT_RESULT(Operation_Identify_Transaction_Misplaced, 9);

	/// Validation failed because operation identify transaction aggregated with end execute transaction.
	DEFINE_SUPERCONTRACT_RESULT(Operation_Identify_Transaction_Aggregated_With_End_Execute, 10);

	/// Validation failed because execution count exceeded limit.
	DEFINE_SUPERCONTRACT_RESULT(Execution_Count_Exceeded_Limit, 11);

	/// Validation failed because execution is in-progress.
	DEFINE_SUPERCONTRACT_RESULT(Execution_Is_In_Progress, 12);

	/// Validation failed because execution is not in-progress.
	DEFINE_SUPERCONTRACT_RESULT(Execution_Is_Not_In_Progress, 13);

	/// Validation failed because drive key is invalid.
	DEFINE_SUPERCONTRACT_RESULT(Invalid_Drive_Key, 14);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
