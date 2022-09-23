/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
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

		/// Height at which registry was created/modified
		Height RefHeight;
	};
}}
