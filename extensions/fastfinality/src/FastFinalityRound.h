/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/utils/NetworkTime.h"

namespace catapult { namespace fastfinality {

#pragma pack(push, 1)

	struct FastFinalityRound {
		int64_t Round = -1;
		utils::TimePoint RoundStart = utils::ToTimePoint(utils::NetworkTime());
		uint64_t RoundTimeMillis = 0u;
	};

#pragma pack(pop)
}}