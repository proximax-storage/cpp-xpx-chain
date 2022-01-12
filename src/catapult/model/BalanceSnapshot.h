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
#include "catapult/types.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a balance snapshot.
	struct BalanceSnapshot {
		/// Balance amount when snapshot was done.
		catapult::Amount Amount;

		/// Locked balance amount when snapshot was done.
		catapult::Amount LockedAmount;

		/// Height of balance when snapshot was done.
		Height BalanceHeight;

		catapult::Amount GetEffectiveAmount() const
		{
			return LockedAmount + Amount;
		}
	};

#pragma pack(pop)
}}
