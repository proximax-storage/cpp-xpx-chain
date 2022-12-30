/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::AutomaticExecutionsReplenishmentNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(AutomaticExecutionsReplenishment, [](const Notification& notification, const ValidatorContext& context) {

		const auto& contractCache = context.Cache.sub<cache::SuperContractCache>();

		auto contractIt = contractCache.find(notification.ContractKey);
		const auto& pContractEntry = contractIt.get();

		if (pContractEntry.deploymentStatus() == state::DeploymentStatus::IN_PROGRESS) {
			return Failure_SuperContract_Deployment_In_Progress;
		}

		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::SuperContractConfiguration>();
		if (static_cast<uint64_t>(pContractEntry.automaticExecutionsInfo().AutomatedExecutionsNumber) +
					notification.Number >
			pluginConfig.MaxAutoExecutions) {
			return Failure_SuperContract_Max_Auto_Executions_Number_Exceeded;
		}

		return ValidationResult::Success;
	})

}}
