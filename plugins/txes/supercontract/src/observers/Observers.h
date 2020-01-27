/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/observers/ObserverTypes.h"
#include "src/config/SuperContractConfiguration.h"
#include "src/model/SuperContractNotifications.h"
#include "src/state/SuperContractEntry.h"
#include "src/cache/SuperContractCache.h"

namespace catapult { namespace observers {

	/// Observes changes triggered by deploy notifications.
	DECLARE_OBSERVER(Deploy, model::DeployNotification<1>)();
}}
