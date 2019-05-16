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
#include "catapult/utils/UnresolvedAddress.h"
#include "src/state/MetadataUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
        struct AddressTraits {
            using ModifyMetadataNotification = ModifyAddressMetadataNotification;
            using ModifyMetadataValueNotification = ModifyAddressMetadataValueNotification;
        };

        struct MosaicTraits {
            using ModifyMetadataNotification = ModifyMosaicMetadataNotification ;
            using ModifyMetadataValueNotification = ModifyMosaicMetadataValueNotification;
        };

        struct NamespaceTraits {
            using ModifyMetadataNotification = ModifyNamespaceMetadataNotification ;
            using ModifyMetadataValueNotification = ModifyNamespaceMetadataValueNotification;
        };

        template<typename TTraits>
        class Publisher {
        public:
            template<typename TTransaction>
            static void Publish(const TTransaction& transaction, NotificationSubscriber& sub) {
                sub.notify(MetadataTypeNotification(transaction.MetadataType));
                sub.notify(CreateMetadataModificationsNotification<TTransaction>(transaction));

                std::vector<const model::MetadataModification*> modifications;

                for (const auto& modification : transaction.Transactions())
                    modifications.emplace_back(&modification);

                if (!modifications.empty())
                    sub.notify(MetadataModificationsNotification(
                        state::GetHash(state::ToVector(transaction.MetadataId), transaction.MetadataType),
                        modifications));

                for (const auto& modification : transaction.Transactions()) {
                    sub.notify(ModifyMetadataFieldNotification(
                        modification.ModificationType,
                        modification.KeySize, modification.KeyPtr(),
                        modification.ValueSize, modification.ValuePtr()));

                    sub.notify(typename TTraits::ModifyMetadataValueNotification(
                        transaction.MetadataId, transaction.MetadataType,
                        modification.ModificationType,
                        modification.KeySize, modification.KeyPtr(),
                        modification.ValueSize, modification.ValuePtr()));
                }
            }

        private:
            template<typename TTransaction>
            static auto CreateMetadataModificationsNotification(const TTransaction& transaction) {
                return typename TTraits::ModifyMetadataNotification(transaction.Signer, transaction.MetadataId);
            }
        };
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(AddressMetadata, Publisher<AddressTraits>::Publish)
	DEFINE_TRANSACTION_PLUGIN_FACTORY(MosaicMetadata, Publisher<MosaicTraits>::Publish)
	DEFINE_TRANSACTION_PLUGIN_FACTORY(NamespaceMetadata, Publisher<NamespaceTraits>::Publish)
}}
