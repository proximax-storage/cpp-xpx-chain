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

#include <src/catapult/config_holder/LocalNodeConfigurationHolder.h>
#include "MosaicDefinitionTransactionPlugin.h"
#include "src/model/MosaicDefinitionTransaction.h"
#include "src/model/MosaicNotifications.h"
#include "src/config/MosaicConfiguration.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "catapult/model/Address.h"
#include "catapult/plugins/PluginUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		MosaicRentalFeeConfiguration ToMosaicRentalFeeConfiguration(
			const model::NetworkInfo& network,
			UnresolvedMosaicId currencyMosaicId,
			const config::MosaicConfiguration& config) {
			MosaicRentalFeeConfiguration rentalFeeConfig;
			rentalFeeConfig.SinkPublicKey = config.MosaicRentalFeeSinkPublicKey;
			rentalFeeConfig.CurrencyMosaicId = currencyMosaicId;
			rentalFeeConfig.Fee = config.MosaicRentalFee;
			rentalFeeConfig.NemesisPublicKey = network.PublicKey;

			// sink address is already resolved but needs to be passed as unresolved into notification
			auto sinkAddress = PublicKeyToAddress(rentalFeeConfig.SinkPublicKey, network.Identifier);
			std::memcpy(rentalFeeConfig.SinkAddress.data(), sinkAddress.data(), sinkAddress.size());
			return rentalFeeConfig;
		}

		template<typename TTransaction>
		auto CreatePublisher(const std::shared_ptr<config::LocalNodeConfigurationHolder>& pConfigHolder) {
			return [pConfigHolder](const TTransaction& transaction, const Height& associatedHeight, NotificationSubscriber& sub) {
				const model::BlockChainConfiguration& blockChainConfig = pConfigHolder->Config(associatedHeight).BlockChain;
				const auto& pluginConfig = blockChainConfig.GetPluginConfiguration<config::MosaicConfiguration>(PLUGIN_NAME(mosaic));
				auto currencyMosaicId = model::GetUnresolvedCurrencyMosaicId(blockChainConfig);
				auto config = ToMosaicRentalFeeConfiguration(blockChainConfig.Network, currencyMosaicId, pluginConfig);

				switch (transaction.EntityVersion()) {
				case 3:
					// 1. sink account notification
					sub.notify(AccountPublicKeyNotification<1>(config.SinkPublicKey));

					// 2. rental fee charge
					// a. exempt the nemesis account
					if (config.NemesisPublicKey != transaction.Signer) {
						sub.notify(BalanceTransferNotification<1>(transaction.Signer, config.SinkAddress, config.CurrencyMosaicId, config.Fee));
						sub.notify(MosaicRentalFeeNotification<1>(transaction.Signer, config.SinkAddress, config.CurrencyMosaicId, config.Fee));
					}

					// 3. registration
					sub.notify(MosaicNonceNotification<1>(transaction.Signer, transaction.MosaicNonce, transaction.MosaicId));
					sub.notify(MosaicPropertiesNotification<1>(transaction.PropertiesHeader, transaction.PropertiesPtr()));
					sub.notify(MosaicDefinitionNotification<1>(
						transaction.Signer,
						transaction.MosaicId,
						ExtractAllProperties(transaction.PropertiesHeader, transaction.PropertiesPtr())));
					break;

				default:
					CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of MosaicDefinitionTransaction", transaction.EntityVersion());
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(MosaicDefinition, CreatePublisher, std::shared_ptr<config::LocalNodeConfigurationHolder>)
}}
