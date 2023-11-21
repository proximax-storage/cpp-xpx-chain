/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SwapOperation.h"
#include "catapult/utils/MacroBasedEnumIncludes.h"

namespace catapult { namespace utils {

#define DEFINE_ENUM SwapOperation
#define ENUM_LIST SWAP_OPERATION_LIST
#include "catapult/utils/MacroBasedEnum.h"
#undef ENUM_LIST
#undef DEFINE_ENUM

}}
