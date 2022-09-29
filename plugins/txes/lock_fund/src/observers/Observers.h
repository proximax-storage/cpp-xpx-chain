/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/LockFundNotifications.h"
#include "catapult/observers/ObserverTypes.h"

namespace catapult { namespace config { class CatapultDirectory; } }

namespace catapult { namespace observers {

	/// Observes LockFund notifications and moves mosaics to the respective balance.
	DECLARE_OBSERVER(LockFundTransfer, model::LockFundTransferNotification<1>)();

	/// Observes Block notifications and processes existing requests for the given block height. Installs and setups the plugin
	DECLARE_OBSERVER(LockFundBlock, model::BlockNotification<1>)();

	/// Observes LockFund Cancel unlock notifications and removes the given unlock request.
	DECLARE_OBSERVER(LockFundCancelUnlock, model::LockFundCancelUnlockNotification<1>)();

}}
