/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommitteePhase.h"
#include "catapult/utils/MacroBasedEnumIncludes.h"

namespace catapult { namespace fastfinality {

#define DEFINE_ENUM CommitteePhase
#define ENUM_LIST COMMITTEE_PHASE_LIST
#include "catapult/utils/MacroBasedEnum.h"
#undef ENUM_LIST
#undef DEFINE_ENUM

}}
