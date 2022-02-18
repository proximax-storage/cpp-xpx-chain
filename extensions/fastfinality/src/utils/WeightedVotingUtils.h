/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "fastfinality/src/WeightedVotingChainPackets.h"

namespace catapult { namespace fastfinality {

	RawBuffer CommitteeMessageDataBuffer(const CommitteeMessage &message);
}}