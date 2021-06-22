/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "VerificationPaymentTransactionPlugin.h"
#include "catapult/model/StorageNotifications.h"
#include "src/model/VerificationPaymentTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(VerificationPaymentNotification<1>(
						transaction.Signer,
						transaction.DriveKey,
						transaction.VerificationFeeAmount
				));
				break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of VerificationPaymentTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(VerificationPayment, Default, Publish)
}}
