/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EndOperationTransactionPlugin.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/model/EndOperationTransaction.h"
#include "TransactionPublishers.h"

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			EndOperationPublisher(transaction, sub, "EndOperationTransaction");
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(EndOperation, Only_Embeddable, Publish)
}}
