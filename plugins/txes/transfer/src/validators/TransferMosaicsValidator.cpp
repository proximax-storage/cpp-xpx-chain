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

#include "Validators.h"
#include "src/config/TransferConfiguration.h"

namespace catapult { namespace validators {

	using Notification = model::TransferMosaicsNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(TransferMosaics, Notification)(const std::shared_ptr<config::LocalNodeConfigurationHolder>& pConfigHolder) {
		return MAKE_STATEFUL_VALIDATOR(TransferMosaics, [pConfigHolder](const auto& notification, const auto& context) {
			const model::BlockChainConfiguration& blockChainConfig = pConfigHolder->Config(context.Height).BlockChain;
			const auto& pluginConfig = blockChainConfig.GetPluginConfiguration<config::TransferConfiguration>(PLUGIN_NAME_HASH(transfer));

			// check strict ordering of mosaics
			if (1 > notification.MosaicsCount)
				return ValidationResult::Success;

			if (pluginConfig.MaxMosaicsSize < notification.MosaicsCount)
				return Failure_Transfer_Too_Many_Mosaics;

			auto pMosaics = notification.MosaicsPtr;
			UnresolvedMosaicId lastMosaicId;
			for (auto i = 0u; i < notification.MosaicsCount; ++i) {
				auto currentMosaicId = pMosaics[i].MosaicId;
				if (i != 0 && lastMosaicId >= currentMosaicId)
					return Failure_Transfer_Out_Of_Order_Mosaics;

				if (1 > pMosaics[i].Amount.unwrap())
					return Failure_Transfer_Invalid_Amount;

				lastMosaicId = currentMosaicId;
			}

			return ValidationResult::Success;
		});
	}
}}
