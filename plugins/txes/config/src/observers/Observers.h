/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/CatapultConfigNotifications.h"
#include "catapult/observers/ObserverTypes.h"
#include "catapult/plugins/PluginManager.h"

namespace catapult { namespace observers {

	/// Observes changes triggered by catapult config notifications
	DECLARE_OBSERVER(CatapultConfig, model::CatapultConfigNotification<1>)();

	/// Observes block notifications and applies new catapult configuration if there is any
	DECLARE_OBSERVER(CatapultConfigApply, model::BlockNotification<1>)(plugins::PluginManager& manager);
}}
