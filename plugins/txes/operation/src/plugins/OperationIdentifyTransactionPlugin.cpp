/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "OperationIdentifyTransactionPlugin.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/model/OperationIdentifyTransaction.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber&) {
			switch (transaction.EntityVersion()) {
				case 1: {
					// Noop
					break;
				}
				default:
					CATAPULT_LOG(debug) << "invalid version of OperationIdentifyTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(OperationIdentify, Default, Publish)
}}
