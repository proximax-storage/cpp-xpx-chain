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

/// Defines an operation validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_OPERATION_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, Operation, DESCRIPTION, CODE, None)

	/// Validation failed because plugin configuration data is malformed.
	DEFINE_OPERATION_RESULT(Plugin_Config_Malformed, 1);

	/// Validation failed because operation token is invalid.
	DEFINE_OPERATION_RESULT(Token_Invalid, 2);

	/// Validation failed because operation expired.
	DEFINE_OPERATION_RESULT(Expired, 3);

	/// Validation failed because operation duration is too long.
	DEFINE_OPERATION_RESULT(Invalid_Duration, 4);

	/// Validation failed because operation executor duplicated.
	DEFINE_OPERATION_RESULT(Executor_Redundant, 5);

	/// Validation failed because mosaic duplicated.
	DEFINE_OPERATION_RESULT(Mosaic_Redundant, 6);

	/// Validation failed because mosaic is invalid (not found).
	DEFINE_OPERATION_RESULT(Mosaic_Invalid, 7);

	/// Validation failed because mosaic amount is zero.
	DEFINE_OPERATION_RESULT(Zero_Mosaic_Amount, 8);

	/// Validation failed because mosaic amount is less than available.
	DEFINE_OPERATION_RESULT(Invalid_Mosaic_Amount, 9);

	/// Validation failed because executor not registered with operation.
	DEFINE_OPERATION_RESULT(Invalid_Executor, 10);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
