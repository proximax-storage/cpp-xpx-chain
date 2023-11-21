/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/OperationCache.h"
#include "plugins/txes/lock_shared/src/observers/ExpiredLockInfoObserver.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(ExpiredOperation, model::BlockNotification<1>, [](const auto&, auto& context) {
		ExpiredLockInfoObserver<cache::OperationCache>(context, [](const auto& operationEntry) { return operationEntry.Account; });
	});
}}
