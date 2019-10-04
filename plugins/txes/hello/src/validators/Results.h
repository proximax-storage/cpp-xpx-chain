/**
*** FOR TRAINING PURPOSES ONLY
**/

#pragma once
#ifndef CUSTOM_RESULT_DEFINITION
#include "catapult/validators/ValidationResult.h"

namespace catapult { namespace validators {

#endif
/// Defines a transfer validation result with \a DESCRIPTION and \a CODE.
#define DEFINE_HELLO_RESULT(DESCRIPTION, CODE) DEFINE_VALIDATION_RESULT(Failure, Hello, DESCRIPTION, CODE, None)

        /// Validation failed because the message count is too large or too low
        // Please refer to as Failure_Hello_MessageCount_Invalid when using the definition
        DEFINE_HELLO_RESULT(MessageCount_Invalid, 1);

        /// Validation failed because plugin configuration data is malformed. This is required
        DEFINE_HELLO_RESULT(Plugin_Config_Malformed, 2);

#ifndef CUSTOM_RESULT_DEFINITION
    }}
#endif
