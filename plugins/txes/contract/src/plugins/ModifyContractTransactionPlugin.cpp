/**
*** Copyright (c) 2018-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
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
		void Publish(const TTransaction& transaction, NotificationSubscriber& sub) {
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
						sub.notify(ModifyMultisigNewCosignerNotification(
						    transaction.Signer, pModification->CosignatoryPublicKey, transaction.EntityVersion()));
						addedVerifierKeys.insert(pModification->CosignatoryPublicKey);
					}
				}

				sub.notify(ModifyMultisigCosignersNotification(
					transaction.Signer, transaction.VerifierModificationCount, transaction.VerifierModificationsPtr(),
					transaction.EntityVersion()));

				if (!addedVerifierKeys.empty())
					sub.notify(AddressInteractionNotification(
					    transaction.Signer, transaction.Type, {}, addedVerifierKeys, transaction.EntityVersion()));
			}

			sub.notify(ModifyContractNotification(
				transaction.DurationDelta,
				transaction.Signer,
				transaction.Hash,
				transaction.CustomerModificationCount,
				transaction.CustomerModificationsPtr(),
				transaction.ExecutorModificationCount,
				transaction.ExecutorModificationsPtr(),
				transaction.VerifierModificationCount,
				transaction.VerifierModificationsPtr(),
				transaction.EntityVersion()));

			if (!reputationModificationKeys.empty())
				sub.notify(*ReputationUpdateNotification::CreateReputationUpdateNotification(
				    reputationModificationKeys, transaction.EntityVersion()));
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(ModifyContract, Publish)
}}
