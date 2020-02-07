/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/EntityHasher.h"
#include "catapult/model/NotificationSubscriber.h"
#include "src/model/OperationNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	template<typename TTransaction>
	void StartOperationPublisher(const TTransaction& transaction, NotificationSubscriber& sub, const GenerationHash& generationHash, const std::string& transactionName) {
		switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(OperationDurationNotification<1>(transaction.Duration));
				sub.notify(OperationMosaicNotification<1>(transaction.MosaicsPtr(), transaction.MosaicCount));
				auto operationToken = CalculateHash(transaction, generationHash);
				sub.notify(StartOperationNotification<1>(
					operationToken,
					transaction.Signer,
					transaction.ExecutorsPtr(),
					transaction.ExecutorCount,
					transaction.MosaicsPtr(),
					transaction.MosaicCount,
					transaction.Duration));
				auto pMosaic = transaction.MosaicsPtr();
				for (auto i = 0u; i < transaction.MosaicCount; ++i, ++pMosaic) {
					sub.notify(BalanceDebitNotification<1>(transaction.Signer, pMosaic->MosaicId, pMosaic->Amount));
				}

				break;
			}
			default:
				CATAPULT_LOG(debug) << "invalid version of " << transactionName << ": " << transaction.EntityVersion();
		}
	}

	template<typename TTransaction>
	void EndOperationPublisher(const TTransaction& transaction, NotificationSubscriber& sub, const std::string& transactionName) {
		switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(OperationMosaicNotification<1>(transaction.MosaicsPtr(), transaction.MosaicCount));
				sub.notify(EndOperationNotification<1>(
					transaction.Signer,
					transaction.OperationToken,
					transaction.MosaicsPtr(),
					transaction.MosaicCount));
				auto pMosaic = transaction.MosaicsPtr();
				for (auto i = 0u; i < transaction.MosaicCount; ++i, ++pMosaic) {
					sub.notify(BalanceCreditNotification<1>(transaction.Signer, pMosaic->MosaicId, pMosaic->Amount));
				}

				break;
			}
			default:
				CATAPULT_LOG(debug) << "invalid version of " << transactionName << ": " << transaction.EntityVersion();
		}
	}
}}
