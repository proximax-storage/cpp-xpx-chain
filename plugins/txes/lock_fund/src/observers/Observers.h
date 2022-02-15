/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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
#include "src/model/LockFundNotifications.h"
#include "catapult/observers/ObserverTypes.h"

namespace catapult { namespace config { class CatapultDirectory; } }

namespace catapult { namespace observers {

	/// Observes LockFund notifications and moves mosaics to the respective balance.
	DECLARE_OBSERVER(LockFundTransfer, model::LockFundTransferNotification<1>)();

	/// Observes Block notifications and processes existing requests for the given block height.
	DECLARE_OBSERVER(LockFundBlock, model::BlockNotification<1>)();

	/// Observes LockFund Cancel unlock notifications and removes the given unlock request.
	DECLARE_OBSERVER(LockFundCancelUnlock, model::LockFundCancelUnlockNotification<1>)();
}}
