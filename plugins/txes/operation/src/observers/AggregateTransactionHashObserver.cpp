/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "AggregateTransactionHashObserver.h"
#include "src/model/EndOperationTransaction.h"
#include "src/model/OperationIdentifyTransaction.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(AggregateTransactionHash, model::AggregateTransactionHashNotification<1>, [](const auto& notification, auto& context) {
		AggregateTransactionHashObserver<model::EmbeddedOperationIdentifyTransaction>(notification, context);
		AggregateTransactionHashObserver<model::EmbeddedEndOperationTransaction>(notification, context);
	});
}}
