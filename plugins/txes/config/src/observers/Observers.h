/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/NetworkConfigNotifications.h"
#include "catapult/observers/ObserverTypes.h"

namespace catapult { namespace observers {

	/// Observes changes triggered by network config notifications
	DECLARE_OBSERVER(NetworkConfig, model::NetworkConfigNotification<1>)();
	DECLARE_OBSERVER(NetworkConfigV2, model::NetworkConfigNotification<2>)();
}}
