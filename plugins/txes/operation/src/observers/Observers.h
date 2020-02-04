/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/OperationNotifications.h"
#include "catapult/model/Notifications.h"
#include "catapult/observers/ObserverTypes.h"

namespace catapult { namespace observers {

	/// Observes changes triggered by start operation notifications.
	DECLARE_OBSERVER(StartOperation, model::StartOperationNotification<1>)();

	/// Observes changes triggered by completed operation notifications.
	DECLARE_OBSERVER(CompletedOperation, model::CompletedOperationNotification<1>)();

	/// Observes changes triggered by block notifications.
	DECLARE_OBSERVER(ExpiredOperation, model::BlockNotification<1>)();
}}
