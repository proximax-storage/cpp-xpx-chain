/**
*** Copyright (c) 2016-present,
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

#include "ModifyMultisigAccountTransactionPlugin.h"
#include "src/model/MultisigNotifications.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 3: {
				// 1. cosig changes
				utils::KeySet addedCosignatoryKeys;
				if (0 < transaction.ModificationsCount) {
					// raise new cosigner notifications first because they are used for multisig loop detection
					const auto* pModifications = transaction.ModificationsPtr();
					for (auto i = 0u; i < transaction.ModificationsCount; ++i) {
						if (model::CosignatoryModificationType::Add == pModifications[i].ModificationType) {
							sub.notify(ModifyMultisigNewCosignerNotification<1>(transaction.Signer, pModifications[i].CosignatoryPublicKey));
							addedCosignatoryKeys.insert(pModifications[i].CosignatoryPublicKey);
						}
						// We need to inform user that they are co signers(removed or added)
						sub.notify(AccountPublicKeyNotification<1>(pModifications[i].CosignatoryPublicKey));
					}

					sub.notify(ModifyMultisigCosignersNotification<1>(transaction.Signer, transaction.ModificationsCount, pModifications));
				}

				if (!addedCosignatoryKeys.empty())
					sub.notify(AddressInteractionNotification<1>(transaction.Signer, transaction.Type, {}, addedCosignatoryKeys));

				// 2. setting changes
				sub.notify(ModifyMultisigSettingsNotification<1>(
						transaction.Signer,
						transaction.MinRemovalDelta,
						transaction.MinApprovalDelta,
						transaction.ModificationsCount));
				break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of ModifyMultisigAccountTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(ModifyMultisigAccount, Only_Embeddable, Publish)
}}
