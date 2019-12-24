/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EndDriveVerificationTransactionPlugin.h"
#include "catapult/model/Address.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "src/model/ServiceNotifications.h"
#include "src/model/EndDriveVerificationTransaction.h"
#include "plugins/txes/multisig/src/model/MultisigNotifications.h"
#include "plugins/txes/lock_secret/src/model/LockHashAlgorithm.h"
#include "plugins/txes/lock_secret/src/model/SecretLockNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const NetworkIdentifier& networkIdentifier) {
			return [networkIdentifier](const TTransaction &transaction, const Height&, NotificationSubscriber &sub) {
				switch (transaction.EntityVersion()) {
					case 1: {
						sub.notify(DriveNotification<1>(transaction.Signer, transaction.Type));

						auto failures = transaction.Transactions(EntityContainerErrorPolicy::Suppress);
						auto failureCount = std::distance(failures.cbegin(), failures.cend());
						auto pFailedReplicators = sub.mempool().malloc<Key>(failureCount);
						uint16_t i = 0u;
						for (auto iter = failures.cbegin(); iter != failures.cend(); ++iter) {
							pFailedReplicators[i++] = iter->Replicator;
							sub.notify(FailedBlockHashesNotification<1>(iter->BlockHashCount(), iter->BlockHashesPtr()));
						}

						sub.notify(EndDriveVerificationNotification<1>(
							transaction.Signer,
							failureCount,
							pFailedReplicators));

						sub.notify(ProofPublicationNotification<1>(
							transaction.Signer,
							LockHashAlgorithm::Op_Internal,
							Hash256(),
							extensions::CopyToUnresolvedAddress(PublicKeyToAddress(transaction.Signer, networkIdentifier))));

						auto pModifications = sub.mempool().malloc<CosignatoryModification>(failureCount);
						for (i = 0u; i < failureCount; ++i) {
							pModifications[i] = CosignatoryModification{model::CosignatoryModificationType::Del, pFailedReplicators[i]};
						}
						sub.notify(ModifyMultisigCosignersNotification<1>(transaction.Signer, failureCount, pModifications, true /* AllowMultipleRemove */));

						sub.notify(DriveVerificationPaymentNotification<1>(
							transaction.Signer,
							failureCount,
							pFailedReplicators));

						break;
					}
					default:
						CATAPULT_LOG(debug) << "invalid version of EndDriveVerificationTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(EndDriveVerification, Default, CreatePublisher, NetworkIdentifier)
}}
