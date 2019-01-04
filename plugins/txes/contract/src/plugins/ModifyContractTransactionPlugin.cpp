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
		void Publish(const TTransaction& transaction, const PublisherContext&, NotificationSubscriber& sub) {
			std::vector<CosignatoryModification> reputationModificationKeys;
			if (0 < transaction.ExecutorModificationCount) {
				const auto* pModification = transaction.ExecutorModificationsPtr();
				for (auto i = 0u; i < transaction.ExecutorModificationCount; ++i, ++pModification) {
					reputationModificationKeys.push_back(*pModification);
				}
			}

			if (0 < transaction.VerifierModificationCount) {
				utils::KeySet addedVerifierKeys;
				const auto* pModification = transaction.VerifierModificationsPtr();
				for (auto i = 0u; i < transaction.VerifierModificationCount; ++i, ++pModification) {
					reputationModificationKeys.push_back(*pModification);
					if (model::CosignatoryModificationType::Add == pModification->ModificationType) {
						sub.notify(ModifyMultisigNewCosignerNotification(transaction.Multisig, pModification->CosignatoryPublicKey));
						addedVerifierKeys.insert(pModification->CosignatoryPublicKey);
					}
				}

				sub.notify(ModifyMultisigCosignersNotification(
					transaction.Multisig, transaction.VerifierModificationCount, transaction.VerifierModificationsPtr()));

				if (!addedVerifierKeys.empty())
					sub.notify(AddressInteractionNotification(transaction.Multisig, model::AddressSet{}, addedVerifierKeys));
			}

			sub.notify(ModifyContractNotification(
				transaction.DurationDelta,
				transaction.Multisig,
				transaction.Hash,
				transaction.CustomerModificationCount,
				transaction.CustomerModificationsPtr(),
				transaction.ExecutorModificationCount,
				transaction.ExecutorModificationsPtr(),
				transaction.VerifierModificationCount,
				transaction.VerifierModificationsPtr()));

			if (!reputationModificationKeys.empty())
				sub.notify(ReputationUpdateNotification(reputationModificationKeys));
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(ModifyContract, Publish)
}}
