/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/config/AggregateConfiguration.h"
#include "src/model/AggregateEntityType.h"

namespace catapult { namespace validators {

	using Notification = model::AggregateTransactionTypeNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(AggregateTransactionType, Notification)(const std::shared_ptr<config::LocalNodeConfigurationHolder>& pConfigHolder) {
		return MAKE_STATEFUL_VALIDATOR(AggregateTransactionType, ([&pConfigHolder](const auto& notification, const auto& context) {
			const model::BlockChainConfiguration& blockChainConfig = pConfigHolder->Config(context.Height).BlockChain;
			const auto& pluginConfig = blockChainConfig.GetPluginConfiguration<config::AggregateConfiguration>("catapult.plugins.aggregate");
			if (notification.Type == model::Entity_Type_Aggregate_Bonded && !pluginConfig.EnableBondedAggregateSupport)
				return Failure_Aggregate_Bonded_Not_Enabled;

			return ValidationResult::Success;
		}));
	}
}}
