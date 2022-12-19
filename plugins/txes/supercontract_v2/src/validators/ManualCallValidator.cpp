/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::ManualCallNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(ManualCall, [](const Notification& notification, const ValidatorContext& context) {

		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::SuperContractConfiguration>();

		if (notification.FileName.size() > pluginConfig.MaxRowSize) {
			return Failure_SuperContract_Max_Row_Size_Exceeded;
		}
		if (notification.FunctionName.size() > pluginConfig.MaxRowSize) {
			return Failure_SuperContract_Max_Row_Size_Exceeded;
		}
		if (notification.ActualArguments.size() > pluginConfig.MaxRowSize) {
			return Failure_SuperContract_Max_Row_Size_Exceeded;
		}

		const auto& contractCache = context.Cache.sub<cache::SuperContractCache>();

		auto contractIt = contractCache.find(notification.ContractKey);
		auto* pContractEntry = contractIt.tryGet();

		if (!pContractEntry) {
			return Failure_SuperContract_Contract_Does_Not_Exist;
		}

		if (pContractEntry->deploymentStatus() == state::DeploymentStatus::IN_PROGRESS) {
			return Failure_SuperContract_Deployment_In_Progress;
		}

		return ValidationResult::Success;
	})

}}
