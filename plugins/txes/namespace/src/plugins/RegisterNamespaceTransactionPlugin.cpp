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

#include "RegisterNamespaceTransactionPlugin.h"
#include "src/config/NamespaceConfiguration.h"
#include "src/model/NamespaceNotifications.h"
#include "src/model/RegisterNamespaceTransaction.h"
#include "catapult/constants.h"
#include "catapult/model/Address.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		static void PublishBalanceTransfer(
				const NamespaceRentalFeeConfiguration& config,
				const TTransaction& transaction,
				NotificationSubscriber& sub) {
			auto rentalFee = config.ChildFee;
			switch (transaction.EntityVersion()) {
			case 2:
				// a. exempt the nemesis account
				if (config.NemesisPublicKey == transaction.Signer)
					return;

				if (transaction.IsRootRegistration()) {
					// b. don't charge fees for eternal namespaces
					if (Eternal_Artifact_Duration == transaction.Duration)
						return;

					rentalFee = Amount(config.RootFeePerBlock.unwrap() * transaction.Duration.unwrap());
				}

				sub.notify(BalanceTransferNotification<1>(transaction.Signer, config.SinkAddress, config.CurrencyMosaicId, rentalFee));
				sub.notify(NamespaceRentalFeeNotification<1>(transaction.Signer, config.SinkAddress, config.CurrencyMosaicId, rentalFee));
				break;

			default:
				CATAPULT_LOG(debug) << "invalid version of RegisterNamespaceTransaction: " << transaction.EntityVersion();
			}
		}

		NamespaceRentalFeeConfiguration ToNamespaceRentalFeeConfiguration(
			model::NetworkIdentifier networkIdentifier,
			const model::NetworkInfo& network,
			UnresolvedMosaicId currencyMosaicId,
			const config::NamespaceConfiguration& config) {
			NamespaceRentalFeeConfiguration rentalFeeConfig;
			rentalFeeConfig.SinkPublicKey = config.NamespaceRentalFeeSinkPublicKey;
			rentalFeeConfig.CurrencyMosaicId = currencyMosaicId;
			rentalFeeConfig.RootFeePerBlock = config.RootNamespaceRentalFeePerBlock;
			rentalFeeConfig.ChildFee = config.ChildNamespaceRentalFee;
			rentalFeeConfig.NemesisPublicKey = network.PublicKey;

			// sink address is already resolved but needs to be passed as unresolved into notification
			auto sinkAddress = PublicKeyToAddress(rentalFeeConfig.SinkPublicKey, networkIdentifier);
			std::memcpy(rentalFeeConfig.SinkAddress.data(), sinkAddress.data(), sinkAddress.size());
			return rentalFeeConfig;
		}

		template<typename TTransaction>
		auto CreatePublisher(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
			return [pConfigHolder](const TTransaction& transaction, const Height& associatedHeight, NotificationSubscriber& sub) {
				const auto& blockChainConfig = pConfigHolder->ConfigAtHeightOrLatest(associatedHeight);
				const auto& immutableConfig = blockChainConfig.Immutable;
				auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(immutableConfig);
				const auto& pluginConfig = blockChainConfig.Network.template GetPluginConfiguration<config::NamespaceConfiguration>();
				auto rentalFeeConfig = ToNamespaceRentalFeeConfiguration(immutableConfig.NetworkIdentifier, blockChainConfig.Network.Info, currencyMosaicId, pluginConfig);

				switch (transaction.EntityVersion()) {
				case 2: {
					// 1. sink account notification
					sub.notify(AccountPublicKeyNotification<1>(rentalFeeConfig.SinkPublicKey));

					// 2. rental fee charge
					PublishBalanceTransfer(rentalFeeConfig, transaction, sub);

					// 3. registration notifications
					sub.notify(NamespaceNotification<1>(transaction.NamespaceType));
					auto parentId = Namespace_Base_Id;
					if (transaction.IsRootRegistration()) {
						using Notification = RootNamespaceNotification<1>;
						sub.notify(Notification(transaction.Signer, transaction.NamespaceId, transaction.Duration));
					} else {
						using Notification = ChildNamespaceNotification<1>;
						sub.notify(Notification(transaction.Signer, transaction.NamespaceId, transaction.ParentId));
						parentId = transaction.ParentId;
					}

					sub.notify(NamespaceNameNotification<1>(
						transaction.NamespaceId,
						parentId,
						transaction.NamespaceNameSize,
						transaction.NamePtr()));
					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of RegisterNamespaceTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(RegisterNamespace, Default, CreatePublisher, std::shared_ptr<config::BlockchainConfigurationHolder>)
}}
