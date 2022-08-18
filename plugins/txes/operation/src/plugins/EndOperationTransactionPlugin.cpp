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
		void Publish(const TTransaction& transaction, const PublishContext&, NotificationSubscriber& sub) {
			EndOperationPublisher(transaction, sub, "EndOperationTransaction");

			switch (transaction.EntityVersion()) {
				case 1: {
					auto pMosaic = transaction.MosaicsPtr();
					for (auto i = 0u; i < transaction.MosaicCount; ++i, ++pMosaic) {
						sub.notify(BalanceCreditNotification<1>(transaction.Signer, pMosaic->MosaicId, pMosaic->Amount));
					}

					break;
				}
				default:
					CATAPULT_LOG(debug) << "invalid version of EndOperationTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(EndOperation, Only_Embeddable, Publish)
}}
