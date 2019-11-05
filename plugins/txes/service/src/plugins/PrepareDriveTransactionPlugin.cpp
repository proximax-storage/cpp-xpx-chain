/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "PrepareDriveTransactionPlugin.h"
#include "plugins/txes/multisig/src/model/MultisigNotifications.h"
#include "src/model/ServiceNotifications.h"
#include "src/model/PrepareDriveTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(ModifyMultisigNewCosignerNotification<1>(transaction.Signer, transaction.Owner));
				// We need to inform user that he is created drive
				sub.notify(AccountPublicKeyNotification<1>(transaction.Owner));

				// TODO: Fix memory leak
				auto modification = new CosignatoryModification{ model::CosignatoryModificationType::Add, transaction.Owner };
				sub.notify(ModifyMultisigCosignersNotification<1>(transaction.Signer, 1, modification));
                sub.notify(ModifyMultisigSettingsNotification<1>(transaction.Signer, 1, 1));

				sub.notify(PrepareDriveNotification<1>(
					transaction.Signer,
					transaction.Owner,
					transaction.Duration,
					transaction.BillingPeriod,
					transaction.BillingPrice,
					transaction.DriveSize,
					transaction.Replicas,
					transaction.MinReplicators,
					transaction.PercentApprovers
				));
                break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of PrepareDriveTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(PrepareDrive, Only_Embeddable, Publish)
}}
