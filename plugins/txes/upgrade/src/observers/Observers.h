/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/BlockchainUpgradeNotifications.h"
#include "catapult/observers/ObserverTypes.h"

namespace catapult { namespace observers {

	/// Observes changes triggered by blockchain upgrade notifications
	DECLARE_OBSERVER(BlockchainUpgrade, model::BlockchainUpgradeVersionNotification<1>)();
}}
