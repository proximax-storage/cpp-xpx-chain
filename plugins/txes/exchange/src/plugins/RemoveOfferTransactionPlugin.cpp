/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "RemoveOfferTransactionPlugin.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/model/RemoveOfferTransaction.h"
#include "src/model/ExchangeNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1:
				sub.notify(RemoveOfferNotification<1>(
					transaction.Signer,
					transaction.BuyOfferCount,
					transaction.BuyOfferHashesPtr(),
					transaction.SellOfferCount,
					transaction.SellOfferHashesPtr()));
				break;

			default:
				CATAPULT_LOG(debug) << "invalid version of RemoveOfferTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(RemoveOffer, Default, Publish)
}}
