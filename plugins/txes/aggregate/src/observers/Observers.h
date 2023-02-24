/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/observers/ObserverTypes.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/model/Notifications.h"

namespace catapult::observers {

	DECLARE_OBSERVER(TransactionFeeCompensation, model::TransactionFeeNotification<1>)();

} // namespace catapult::observers
