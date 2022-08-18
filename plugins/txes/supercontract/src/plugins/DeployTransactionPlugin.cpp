/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DeployTransactionPlugin.h"
#include "src/model/SuperContractNotifications.h"
#include "src/model/DeployTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "plugins/txes/service/src/model/ServiceNotifications.h"
#include "plugins/txes/multisig/src/model/MultisigNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const PublishContext&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(DriveNotification<1>(transaction.DriveKey, transaction.Type));
				sub.notify(ModifyMultisigNewCosignerNotification<1>(transaction.Signer, transaction.DriveKey));
				sub.notify(AccountPublicKeyNotification<1>(transaction.Owner));
				sub.notify(AccountPublicKeyNotification<1>(transaction.DriveKey));

				auto pModification = sub.mempool().malloc(
					CosignatoryModification{model::CosignatoryModificationType::Add, transaction.DriveKey});
				sub.notify(ModifyMultisigCosignersNotification<1>(transaction.Signer, 1, pModification));
				sub.notify(ModifyMultisigSettingsNotification<1>(transaction.Signer, 1, 1, 1));

				sub.notify(DeployNotification<1>(
					transaction.Signer,
					transaction.Owner,
					transaction.DriveKey,
					transaction.FileHash,
					transaction.VmVersion
				));
                break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of DeployTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(Deploy, Only_Embeddable, Publish)
}}
