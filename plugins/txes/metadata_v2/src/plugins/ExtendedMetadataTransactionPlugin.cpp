/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExtendedMetadataTransactionPlugin.h"
#include "src/model/AccountExtendedMetadataTransaction.h"
#include "src/model/MetadataNotifications.h"
#include "src/model/MosaicExtendedMetadataTransaction.h"
#include "src/model/NamespaceExtendedMetadataTransaction.h"
#include "plugins/txes/namespace/src/model/NamespaceNotifications.h"
#include "catapult/model/Address.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		UnresolvedPartialMetadataKey ExtractPartialMetadataKey(const Address& signer, const TTransaction& transaction) {
			return { signer, transaction.TargetKey, transaction.ScopedMetadataKey };
		}

		struct AccountTraits {
			template<typename TTransaction>
			static MetadataTarget ExtractMetadataTarget(const TTransaction&) {
				return { MetadataType::Account, 0 };
			}

			template<typename TTransaction>
			static void RaiseCustomNotifications(const TTransaction& transaction, NotificationSubscriber& sub) {
				sub.notify(AccountPublicKeyNotification<1>(transaction.TargetKey));
			}
		};

		struct MosaicTraits {
			template<typename TTransaction>
			static MetadataTarget ExtractMetadataTarget(const TTransaction& transaction) {
				return { MetadataType::Mosaic, transaction.TargetMosaicId.unwrap() };
			}

			template<typename TTransaction>
			static void RaiseCustomNotifications(const TTransaction& transaction, NotificationSubscriber& sub) {
				sub.notify(MosaicRequiredNotification<1>(transaction.TargetKey, transaction.TargetMosaicId));
			}
		};

		struct NamespaceTraits {
			template<typename TTransaction>
			static MetadataTarget ExtractMetadataTarget(const TTransaction& transaction) {
				return { MetadataType::Namespace, transaction.TargetNamespaceId.unwrap() };
			}

			template<typename TTransaction>
			static void RaiseCustomNotifications(const TTransaction& transaction, NotificationSubscriber& sub) {
				sub.notify(NamespaceRequiredNotification<1>(transaction.TargetKey, transaction.TargetNamespaceId));
			}
		};

		template<typename TTraits>
		class Publisher {
		public:
			template<typename TTransaction>
			static auto CreatePublisher(const std::shared_ptr<config::BlockchainConfigurationHolder> &pConfigHolder) {
				return [pConfigHolder](
							   const TTransaction& transaction,
							   const Height& associatedHeight,
							   NotificationSubscriber& sub) {
					auto networkIdentifier = pConfigHolder->Config(associatedHeight).Immutable.NetworkIdentifier;

				  	auto address = model::PublicKeyToAddress(transaction.Signer, networkIdentifier);
					sub.notify(MetadataSizesNotification<1>(transaction.ValueSizeDelta, transaction.ValueSize));
					sub.notify(MetadataValueNotification<2>(
							ExtractPartialMetadataKey(address, transaction),
							TTraits::ExtractMetadataTarget(transaction),
							transaction.ValueSizeDelta,
							transaction.ValueSize,
							transaction.ValuePtr(),
							transaction.IsValueImmutable));

					TTraits::RaiseCustomNotifications(transaction, sub);
				};
			}
		};
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(AccountExtendedMetadata, Only_Embeddable, Publisher<AccountTraits>::CreatePublisher, std::shared_ptr<config::BlockchainConfigurationHolder>)
	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(MosaicExtendedMetadata, Only_Embeddable, Publisher<MosaicTraits>::CreatePublisher, std::shared_ptr<config::BlockchainConfigurationHolder>)
	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(NamespaceExtendedMetadata, Only_Embeddable, Publisher<NamespaceTraits>::CreatePublisher, std::shared_ptr<config::BlockchainConfigurationHolder>)
}}
