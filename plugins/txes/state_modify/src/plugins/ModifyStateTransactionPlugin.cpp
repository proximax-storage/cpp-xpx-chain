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

#include "ModifyStateTransactionPlugin.h"
#include "src/model/ModifyStateNotifications.h"
#include "src/model/ModifyStateTransaction.h"
#include "src/config/ModifyStateConfiguration.h"
#include "catapult/constants.h"
#include "catapult/cache/CacheConstants.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {


		template<typename TTransaction>
		auto CreatePublisher(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
			return [pConfigHolder](const TTransaction& transaction, const Height& associatedHeight, NotificationSubscriber& sub) {
				const auto& blockChainConfig = pConfigHolder->ConfigAtHeightOrLatest(associatedHeight);
				const auto& immutableConfig = blockChainConfig.Immutable;
				const auto& pluginConfig = blockChainConfig.Network.template GetPluginConfiguration<config::ModifyStateConfiguration>();

				switch (transaction.EntityVersion()) {
				case 1: {
					// 1. sink account notification
					auto cacheName = std::string(reinterpret_cast<const char*>(transaction.CacheNamePtr()), transaction.CacheNameSize);
					sub.notify(ModifyStateNotification<1>(cache::GetCacheIdFromName(cacheName), transaction.SubCacheId, transaction.Signer, transaction.KeyPtr(), transaction.KeySize, transaction.ContentPtr(), transaction.ContentSize));
					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of ModifyStateTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(ModifyState, Default, CreatePublisher, std::shared_ptr<config::BlockchainConfigurationHolder>)
}}
