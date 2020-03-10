/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EndExecuteTransactionPlugin.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/model/EndExecuteTransaction.h"
#include "plugins/txes/operation/src/plugins/TransactionPublishers.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			EndOperationPublisher(transaction, sub, "EndExecuteTransaction");
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(EndExecute, Only_Embeddable, Publish)
}}
