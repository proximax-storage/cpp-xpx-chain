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
/// Defines a download channel validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_STORAGE_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, Storage, DESCRIPTION, CODE, None)

	/// Desired drive size is less than minimal.
	DEFINE_STORAGE_RESULT(Prepare_Too_Small_DriveSize, 1);

	/// Desired number of replicators is less than minimal.
	DEFINE_STORAGE_RESULT(Prepare_Too_Small_ReplicatorCount, 2);

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_STORAGE_RESULT(Plugin_Config_Malformed, 3);

	/// Data modification is not present in activeDataModifications.
	DEFINE_STORAGE_RESULT(No_Active_Data_Modifications, 4);

	/// There are other active data modifications that need to be finished first.
	DEFINE_STORAGE_RESULT(Invalid_Data_Modification_Id, 5);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
