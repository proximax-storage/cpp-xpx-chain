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

/// Defines a metadata validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_METADATA_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, Metadata, DESCRIPTION, CODE, None)

	/// Validation failed because the metadata type is invalid.
	DEFINE_METADATA_RESULT(Invalid_Metadata_Type, 1);

	/// Validation failed because a modification type is invalid.
	DEFINE_METADATA_RESULT(Modification_Type_Invalid, 2);

	/// Validation failed because a modification type is invalid.
	DEFINE_METADATA_RESULT(Modification_Key_Invalid, 3);

	/// Validation failed because a modification type is invalid.
	DEFINE_METADATA_RESULT(Modification_Value_Invalid, 4);

#ifndef CUSTOM_RESULT_DEFINITION
}}
#endif
