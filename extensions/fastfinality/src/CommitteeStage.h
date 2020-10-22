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

	struct CommitteeStage {
		uint16_t Round = 0u;
		CommitteePhase Phase = CommitteePhase::None;
		utils::TimePoint RoundStart;
		uint64_t PhaseTimeMillis = 0u;

		constexpr bool operator<(const CommitteeStage& rhs) const {
			if (Round > rhs.Round) {
				return false;
			} else if (Round < rhs.Round) {
				return true;
			} else {
				return Phase < rhs.Phase;
			}
		}
	};

#pragma pack(pop)
}}