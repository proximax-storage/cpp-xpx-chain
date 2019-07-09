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
#include "catapult/utils/ArraySet.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "src/config/AggregateConfiguration.h"

namespace catapult { namespace validators {

	using Notification = model::AggregateCosignaturesNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(BasicAggregateCosignatures, Notification)(const std::shared_ptr<config::LocalNodeConfigurationHolder>& pConfigHolder) {
		return MAKE_STATEFUL_VALIDATOR(BasicAggregateCosignatures, ([pConfigHolder](const auto& notification, const auto& context) {
			if (0 == notification.TransactionsCount)
				return Failure_Aggregate_No_Transactions;

			const model::BlockChainConfiguration& blockChainConfig = pConfigHolder->Config(context.Height).BlockChain;
			const auto& pluginConfig = blockChainConfig.GetPluginConfiguration<config::AggregateConfiguration>("catapult.plugins.aggregate");
			if (pluginConfig.MaxTransactionsPerAggregate < notification.TransactionsCount)
				return Failure_Aggregate_Too_Many_Transactions;

			if (pluginConfig.MaxCosignaturesPerAggregate < notification.CosignaturesCount + 1)
				return Failure_Aggregate_Too_Many_Cosignatures;

			utils::KeyPointerSet cosigners;
			cosigners.insert(&notification.Signer);
			const auto* pCosignature = notification.CosignaturesPtr;
			for (auto i = 0u; i < notification.CosignaturesCount; ++i) {
				if (!cosigners.insert(&pCosignature->Signer).second)
					return Failure_Aggregate_Redundant_Cosignatures;

				++pCosignature;
			}

			return ValidationResult::Success;
		}));
	}
}}
