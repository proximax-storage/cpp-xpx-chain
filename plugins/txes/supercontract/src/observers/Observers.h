/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/observers/ObserverTypes.h"
#include "src/config/SuperContractConfiguration.h"
#include "src/model/SuperContractNotifications.h"
#include "src/cache/SuperContractCache.h"
#include "plugins/txes/aggregate/src/model/AggregateNotifications.h"

namespace catapult { namespace observers {

	/// Observes changes triggered by deploy notifications.
	DECLARE_OBSERVER(Deploy, model::DeployNotification<1>)();

	/// Observes changes triggered by deploy notifications.
	DECLARE_OBSERVER(EndExecute, model::AggregateCosignaturesNotification<2>)();

	/// Observes changes triggered by aggregate transaction hash notifications.
	DECLARE_OBSERVER(AggregateTransactionHash, model::AggregateTransactionHashNotification<1>)();
}}
