/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "RemoveExchangeOfferTransactionPlugin.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/model/RemoveExchangeOfferTransaction.h"
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
					transaction.OfferCount,
					transaction.OffersPtr()));
				break;
			case 2:
				sub.notify(RemoveOfferNotification<2>(
					transaction.Signer,
					transaction.OfferCount,
					transaction.OffersPtr()));
				break;

			default:
				CATAPULT_LOG(debug) << "invalid version of RemoveOfferTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(RemoveExchangeOffer, Default, Publish)
}}
