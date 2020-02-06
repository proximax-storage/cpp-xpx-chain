/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StartOperationTransactionPlugin.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/model/StartOperationTransaction.h"
#include "TransactionPublishers.h"

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction &transaction, const Height&, NotificationSubscriber &sub) {
				StartOperationPublisher(transaction, sub, config.GenerationHash, "StartOperationTransaction");
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(StartOperation, Default, CreatePublisher, config::ImmutableConfiguration)
}}
