/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataTransactionPlugin.h"
#include "src/model/AddressMetadataTransaction.h"
#include "src/model/MosaicMetadataTransaction.h"
#include "src/model/NamespaceMetadataTransaction.h"
#include "src/model/MetadataNotifications.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/state/MetadataUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

    namespace {
        struct AddressTraits {
            using ModifyMetadataNotification = ModifyAddressMetadataNotification_v1;
            using ModifyMetadataValueNotification = ModifyAddressMetadataValueNotification_v1;
        };

        struct MosaicTraits {
            using ModifyMetadataNotification = ModifyMosaicMetadataNotification_v1 ;
            using ModifyMetadataValueNotification = ModifyMosaicMetadataValueNotification_v1;
        };

        struct NamespaceTraits {
            using ModifyMetadataNotification = ModifyNamespaceMetadataNotification_v1 ;
            using ModifyMetadataValueNotification = ModifyNamespaceMetadataValueNotification_v1;
        };

        template<typename TTraits>
        class Publisher {
        public:
            template<typename TTransaction>
            static void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					sub.notify(MetadataTypeNotification<1>(transaction.MetadataType));
					sub.notify(CreateMetadataModificationsNotification<TTransaction>(transaction));

					std::vector<const model::MetadataModification*> modifications;
					for (const auto& modification : transaction.Transactions())
						modifications.emplace_back(&modification);

					if (!modifications.empty())
						sub.notify(MetadataModificationsNotification<1>(
							state::GetHash(state::ToVector(transaction.MetadataId), transaction.MetadataType),
							modifications));

					for (const auto& modification : transaction.Transactions()) {
						sub.notify(ModifyMetadataFieldNotification<1>(
							modification.ModificationType,
							modification.KeySize, modification.KeyPtr(),
							modification.ValueSize, modification.ValuePtr()));

						sub.notify(typename TTraits::ModifyMetadataValueNotification(
							transaction.MetadataId, transaction.MetadataType,
							modification.ModificationType,
							modification.KeySize, modification.KeyPtr(),
							modification.ValueSize, modification.ValuePtr()));
					}
					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of MetadataTransaction: " << transaction.EntityVersion();
				}
            }

        private:
            template<typename TTransaction>
            static auto CreateMetadataModificationsNotification(const TTransaction& transaction) {
                return typename TTraits::ModifyMetadataNotification(transaction.Signer, transaction.MetadataId);
            }
        };
    }

    DEFINE_TRANSACTION_PLUGIN_FACTORY(AddressMetadata, Default, Publisher<AddressTraits>::Publish)
    DEFINE_TRANSACTION_PLUGIN_FACTORY(MosaicMetadata, Default, Publisher<MosaicTraits>::Publish)
    DEFINE_TRANSACTION_PLUGIN_FACTORY(NamespaceMetadata, Default, Publisher<NamespaceTraits>::Publish)
}}
