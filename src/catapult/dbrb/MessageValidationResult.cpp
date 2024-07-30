/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MessageValidationResult.h"
#include "catapult/utils/MacroBasedEnumIncludes.h"

namespace catapult { namespace dbrb {

#define DEFINE_ENUM MessageValidationResult
#define EXPLICIT_VALUE_ENUM
#define ENUM_LIST MESSAGE_VALIDATION_RESULT_LIST
#include "catapult/utils/MacroBasedEnum.h"
#undef ENUM_LIST
#undef EXPLICIT_VALUE_ENUM
#undef DEFINE_ENUM
}}
