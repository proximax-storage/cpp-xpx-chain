/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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

#include "MetadataTransactionPlugin.h"
#include "src/model/AccountMetadataTransaction.h"
#include "src/model/MetadataNotifications.h"
#include "src/model/MosaicMetadataTransaction.h"
#include "src/model/NamespaceMetadataTransaction.h"
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
							   const PublishContext& context,
							   NotificationSubscriber& sub) {
					auto networkIdentifier = pConfigHolder->Config(context.AssociatedHeight).Immutable.NetworkIdentifier;

				  	auto address = model::PublicKeyToAddress(transaction.Signer, networkIdentifier);
					sub.notify(MetadataSizesNotification<1>(transaction.ValueSizeDelta, transaction.ValueSize));
					sub.notify(MetadataValueNotification<1>(
						ExtractPartialMetadataKey(address, transaction),
						TTraits::ExtractMetadataTarget(transaction),
						transaction.ValueSizeDelta,
						transaction.ValueSize,
						transaction.ValuePtr()));

					TTraits::RaiseCustomNotifications(transaction, sub);
				};
			}
		};
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(AccountMetadata, Only_Embeddable, Publisher<AccountTraits>::CreatePublisher, std::shared_ptr<config::BlockchainConfigurationHolder>)
	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(MosaicMetadata, Only_Embeddable, Publisher<MosaicTraits>::CreatePublisher, std::shared_ptr<config::BlockchainConfigurationHolder>)
	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(NamespaceMetadata, Only_Embeddable, Publisher<NamespaceTraits>::CreatePublisher, std::shared_ptr<config::BlockchainConfigurationHolder>)
}}
