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

#include "MosaicSupplyChangeTransactionPlugin.h"
#include "src/model/MosaicNotifications.h"
#include "src/model/MosaicSupplyChangeTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		static auto CreatePublisher(const std::shared_ptr<config::BlockchainConfigurationHolder> &pConfigHolder) {
			return [pConfigHolder](
						   const TTransaction& transaction,
						   const PublishContext& associatedHeight,
						   NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 3:
					sub.notify(MosaicRequiredNotification<2>(transaction.Signer, transaction.MosaicId, MosaicRequirementAction::Set));
					sub.notify(MosaicSupplyChangeNotification<2>(transaction.Signer, transaction.MosaicId, transaction.Direction, transaction.Delta));
					break;
				case 2:
					sub.notify(MosaicRequiredNotification<1>(transaction.Signer, transaction.MosaicId));
					sub.notify(MosaicSupplyChangeNotification<1>(
							transaction.Signer, transaction.MosaicId, transaction.Direction, transaction.Delta));
					break;

				default:
					CATAPULT_LOG(debug) << "invalid version of MosaicSupplyChangeTransaction: "
										<< transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(MosaicSupplyChange, Default, CreatePublisher, std::shared_ptr<config::BlockchainConfigurationHolder>)
}}
