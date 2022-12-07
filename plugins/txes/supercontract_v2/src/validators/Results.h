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
#define DEFINE_CONTRACT_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, SuperContract, DESCRIPTION, CODE, None)

	DEFINE_CONTRACT_RESULT(Contract_Does_Not_Exist, 1);

	DEFINE_CONTRACT_RESULT(Deployment_In_Progress, 2);

	DEFINE_CONTRACT_RESULT(Contract_Already_Deployed_On_Drive, 3);

	DEFINE_CONTRACT_RESULT(Plugin_Config_Malformed, 4);

	DEFINE_CONTRACT_RESULT(Invalid_Batch_Id, 5);

	DEFINE_CONTRACT_RESULT(Invalid_T_Proof, 6);

	DEFINE_CONTRACT_RESULT(Invalid_Start_Batch_Id, 7);

	DEFINE_CONTRACT_RESULT(Invalid_Batch_Proof, 8);

	DEFINE_CONTRACT_RESULT(Manual_Calls_Are_Not_Requested, 9);

	DEFINE_CONTRACT_RESULT(Automatic_Calls_Are_Not_Requested, 10);

	DEFINE_CONTRACT_RESULT(Invalid_Call_Id, 11);

	DEFINE_CONTRACT_RESULT(ExecutionWorkIsTooLarge, 12);

	DEFINE_CONTRACT_RESULT(DownloadWorkIsTooLarge, 13);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
