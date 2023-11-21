/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteePhase.h"
#include "catapult/utils/NetworkTime.h"

namespace catapult { namespace fastfinality {

#pragma pack(push, 1)

	struct CommitteeRound {
		int64_t Round = -1;
		CommitteePhase StartPhase = CommitteePhase::None;
		utils::TimePoint RoundStart = utils::ToTimePoint(utils::NetworkTime());
		uint64_t PhaseTimeMillis = 0u;
	};

#pragma pack(pop)
}}