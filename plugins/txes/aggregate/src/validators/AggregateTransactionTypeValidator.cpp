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

	DECLARE_STATELESS_VALIDATOR(AggregateTransactionType, Notification)(const model::BlockChainConfiguration& blockChainConfig) {
		return MAKE_STATELESS_VALIDATOR(AggregateTransactionType, ([&blockChainConfig](const auto& notification) {
			const auto& pluginConfig = blockChainConfig.GetPluginConfiguration<config::AggregateConfiguration>("catapult.plugins.aggregate");
			if (notification.Type == model::Entity_Type_Aggregate_Bonded && !pluginConfig.EnableBondedAggregateSupport)
				return Failure_Aggregate_Bonded_Not_Enabled;

			return ValidationResult::Success;
		}));
	}
}}
