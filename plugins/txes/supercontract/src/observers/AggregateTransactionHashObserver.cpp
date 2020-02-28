/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "plugins/txes/operation/src/observers/AggregateTransactionHashObserver.h"
#include "src/model/EndExecuteTransaction.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(AggregateTransactionHash, model::AggregateTransactionHashNotification<1>, [](const auto& notification, auto& context) {
		AggregateTransactionHashObserver<model::EmbeddedEndExecuteTransaction>(notification, context);
	});
}}
