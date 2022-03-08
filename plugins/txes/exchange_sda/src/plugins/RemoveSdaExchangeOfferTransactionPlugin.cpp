/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "RemoveSdaExchangeOfferTransactionPlugin.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/model/RemoveSdaExchangeOfferTransaction.h"
#include "src/model/SdaExchangeNotifications.h"

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

			default:
				CATAPULT_LOG(debug) << "invalid version of RemoveSdaExchangeOfferTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(RemoveSdaExchangeOffer, Default, Publish)
}}
