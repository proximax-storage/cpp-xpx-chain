/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StakingRecord.h"
#include "catapult/model/Address.h"

namespace catapult { namespace state {

	StakingRecord::StakingRecord(const state::AccountState& state, const MosaicId& harvestingMosaicId, const Height& height, const Height& refHeight)
		: Address(state.Address),
		PublicKey(state.PublicKey),
		RegistryHeight(height),
		RefHeight(refHeight){
		TotalStaked = state.Balances.getLocked(harvestingMosaicId);
	}
}}
