/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "ModifyContractTransactionPlugin.h"
#include "plugins/txes/multisig/src/model/MultisigNotifications.h"
#include "src/model/ModifyContractTransaction.h"
#include "src/model/ContractNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 3: {
				std::vector<const CosignatoryModification*> reputationModificationKeys;
				if (0 < transaction.ExecutorModificationCount) {
					const auto* pModification = transaction.ExecutorModificationsPtr();
					for (auto i = 0u; i < transaction.ExecutorModificationCount; ++i, ++pModification) {
						reputationModificationKeys.emplace_back(pModification);
					}
				}

				if (0 < transaction.VerifierModificationCount) {
					utils::KeySet addedVerifierKeys;
					const auto* pModification = transaction.VerifierModificationsPtr();
					for (auto i = 0u; i < transaction.VerifierModificationCount; ++i, ++pModification) {
						reputationModificationKeys.emplace_back(pModification);
						if (model::CosignatoryModificationType::Add == pModification->ModificationType) {
							sub.notify(ModifyMultisigNewCosignerNotification<1>(transaction.Signer, pModification->CosignatoryPublicKey));
							addedVerifierKeys.insert(pModification->CosignatoryPublicKey);
						}
						// We need to inform verifiers that they are co signers(removed or added)
						sub.notify(AccountPublicKeyNotification<1>(pModification->CosignatoryPublicKey));
					}

					sub.notify(ModifyMultisigCosignersNotification<1>(
						transaction.Signer, transaction.VerifierModificationCount, transaction.VerifierModificationsPtr()));

					if (!addedVerifierKeys.empty())
						sub.notify(AddressInteractionNotification<1>(transaction.Signer, transaction.Type, {}, addedVerifierKeys));
				}

				sub.notify(ModifyContractNotification<1>(
					transaction.DurationDelta,
					transaction.Signer,
					transaction.Hash,
					transaction.CustomerModificationCount,
					transaction.CustomerModificationsPtr(),
					transaction.ExecutorModificationCount,
					transaction.ExecutorModificationsPtr(),
					transaction.VerifierModificationCount,
					transaction.VerifierModificationsPtr()));

				if (!reputationModificationKeys.empty())
					sub.notify(*ReputationUpdateNotification<1>::CreateReputationUpdateNotification(reputationModificationKeys));
				break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of ModifyContractTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(ModifyContract, Default, Publish)
}}
