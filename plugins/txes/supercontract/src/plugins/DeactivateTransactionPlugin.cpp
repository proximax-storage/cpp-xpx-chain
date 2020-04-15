/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DeactivateTransactionPlugin.h"
#include "src/model/SuperContractNotifications.h"
#include "src/model/DeactivateTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "plugins/txes/multisig/src/model/MultisigNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(AccountPublicKeyNotification<1>(transaction.DriveKey));
				sub.notify(SuperContractNotification<1>(transaction.SuperContract, transaction.Type));
				sub.notify(DeactivateNotification<1>(
					transaction.Signer,
					transaction.SuperContract,
					transaction.DriveKey
				));
				auto pModification = sub.mempool().malloc(
					CosignatoryModification{model::CosignatoryModificationType::Del, transaction.DriveKey});
				sub.notify(ModifyMultisigCosignersNotification<1>(transaction.SuperContract, 1, pModification));
                break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of DeactivateTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(Deactivate, Default, Publish)
}}
