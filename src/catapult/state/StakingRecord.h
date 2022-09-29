/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "AccountState.h"
#include "catapult/crypto/Hashes.h"
#include <optional>
#include <bitset>

namespace catapult { namespace state {


	/// Account state data.
	struct StakingRecord {
	public:
		StakingRecord(const state::AccountState& state, const MosaicId& harvestingMosaicId, const Height& height, const Height& RefHeight);
	public:

		/// Address of an account.
		catapult::Address Address;

		/// Public key of an account. Present if PublicKeyHeight > 0.
		catapult::Key PublicKey;

		/// Total amount staked
		Amount TotalStaked;

		/// Height at which registry was created/modified
		Height RegistryHeight;

		/// Reference height based on the reward blocks as calculated by the interval set in lock fund configuration.
		Height RefHeight;
	};
}}
