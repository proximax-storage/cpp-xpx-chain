/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_RESULT_DEFINITION
#include "catapult/validators/ValidationResult.h"

namespace catapult { namespace validators {

#endif
/// Defines a DBRB validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_DBRB_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, Dbrb, DESCRIPTION, CODE, None)

		/// Registration attempt is too early, DBRB process is not yet expired.
		DEFINE_DBRB_RESULT(Process_Not_Expired, 1);

#ifndef CUSTOM_RESULT_DEFINITION
	}}
#endif
