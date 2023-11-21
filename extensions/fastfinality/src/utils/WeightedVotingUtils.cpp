/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingUtils.h"

namespace catapult { namespace fastfinality {

	RawBuffer CommitteeMessageDataBuffer(const CommitteeMessage &message) {
		return {
			reinterpret_cast<const uint8_t *>(&message),
			sizeof(CommitteeMessage) - sizeof(Signature)
		};
	}
}}