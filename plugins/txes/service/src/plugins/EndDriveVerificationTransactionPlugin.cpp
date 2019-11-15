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
						sub.notify(EndDriveVerificationNotification<1>(
							transaction.Signer,
							transaction.FailureCount,
							transaction.FailuresPtr()));

						sub.notify(ProofPublicationNotification<1>(
							transaction.Signer,
							LockHashAlgorithm::Op_Internal,
							Hash256(),
							extensions::CopyToUnresolvedAddress(PublicKeyToAddress(transaction.Signer, networkIdentifier))));

						std::vector<CosignatoryModification> modifications;
						modifications.reserve(transaction.FailureCount);
						auto pFailure = transaction.FailuresPtr();
						for (auto i = 0u; i < transaction.FailureCount; ++i, ++pFailure) {
							modifications.push_back(CosignatoryModification{model::CosignatoryModificationType::Del, pFailure->Replicator});
						}
						sub.notify(ModifyMultisigCosignersNotification<1>(transaction.Signer, transaction.FailureCount, modifications.data()));

						sub.notify(DriveVerificationPaymentNotification<1>(
							transaction.Signer,
							transaction.FailureCount,
							transaction.FailuresPtr()));

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
