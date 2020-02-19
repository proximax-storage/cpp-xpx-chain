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
	DEFINE_SUPERCONTRACT_RESULT(File_Is_Not_Exist, 3);

	/// Someone try to remove file of super contract.
	DEFINE_SUPERCONTRACT_RESULT(Remove_Super_Contract_File, 4);

	/// Drive already ended, so you can create super contract on this drive.
	DEFINE_SUPERCONTRACT_RESULT(Drive_Has_Ended, 5);

	/// Super contract is not exist.
	DEFINE_SUPERCONTRACT_RESULT(SuperContract_Is_Not_Exist, 6);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
