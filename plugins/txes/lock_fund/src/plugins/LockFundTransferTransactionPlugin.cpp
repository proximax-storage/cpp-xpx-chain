/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LockFundTransferTransactionPlugin.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/model/LockFundTransferTransaction.h"
#include "src/model/LockFundNotifications.h"
using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		constexpr uint8_t Mosaic_Flags_Lockable = 0x10;
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const PublishContext&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(AccountPublicKeyNotification<1>(transaction.Signer));
				const auto *pMosaics = transaction.MosaicsPtr();
				std::vector<UnresolvedMosaic> mosaics;
				for (auto i = 0u; i < transaction.MosaicsCount; ++i) {
					mosaics.push_back(pMosaics[i]);
					sub.notify(MosaicRequiredNotification<2>(transaction.Signer, pMosaics[i].MosaicId, MosaicRequirementAction::Unset,  Mosaic_Flags_Lockable));
				}
				sub.notify(LockFundTransferNotification<1>(transaction.Signer, transaction.Duration, mosaics, transaction.Action));
				break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of LockFundTransaction: " << transaction.EntityVersion();
			}
		}
	}
	DEFINE_TRANSACTION_PLUGIN_FACTORY(LockFundTransfer, Default, Publish)
}}
