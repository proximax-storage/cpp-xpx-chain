/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataV1TransactionPlugin.h"
#include "src/model/AddressMetadataV1Transaction.h"
#include "src/model/MosaicMetadataV1Transaction.h"
#include "src/model/NamespaceMetadataV1Transaction.h"
#include "src/model/MetadataV1Notifications.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/state/MetadataV1Utils.h"

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
					sub.notify(MetadataV1TypeNotification<1>(transaction.MetadataType));
					sub.notify(CreateMetadataModificationsNotification<TTransaction>(transaction));

					std::vector<const model::MetadataV1Modification*> modifications;
					for (const auto& modification : transaction.Transactions())
						modifications.emplace_back(&modification);

					if (!modifications.empty())
						sub.notify(MetadataV1ModificationsNotification<1>(
							state::GetHash(state::ToVector(transaction.MetadataId), transaction.MetadataType),
							modifications));

					for (const auto& modification : transaction.Transactions()) {
						sub.notify(ModifyMetadataV1FieldNotification<1>(
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

    DEFINE_TRANSACTION_PLUGIN_FACTORY(AddressMetadataV1, Default, Publisher<AddressTraits>::Publish)
    DEFINE_TRANSACTION_PLUGIN_FACTORY(MosaicMetadataV1, Default, Publisher<MosaicTraits>::Publish)
    DEFINE_TRANSACTION_PLUGIN_FACTORY(NamespaceMetadataV1, Default, Publisher<NamespaceTraits>::Publish)
}}
