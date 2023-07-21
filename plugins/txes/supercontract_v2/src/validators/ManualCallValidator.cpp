/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

    using Notification = model::ManualCallNotification<1>;

    DEFINE_STATEFUL_VALIDATOR(ManualCall, [](const Notification& notification, const ValidatorContext& context) {

        const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::SuperContractV2Configuration>();

        if (notification.FileName.size() > pluginConfig.MaxRowSize) {
            return Failure_SuperContract_v2_Max_Row_Size_Exceeded;
        }
        if (notification.FunctionName.size() > pluginConfig.MaxRowSize) {
            return Failure_SuperContract_v2_Max_Row_Size_Exceeded;
        }
        if (notification.ActualArguments.size() > pluginConfig.MaxRowSize) {
            return Failure_SuperContract_v2_Max_Row_Size_Exceeded;
        }

        if (notification.ServicePayments.size() > pluginConfig.MaxServicePaymentsSize) {
            return Failure_SuperContract_v2_Max_Service_Payments_Size_Exceeded;
        }

        const auto& contractCache = context.Cache.sub<cache::SuperContractCache>();

        auto contractIt = contractCache.find(notification.ContractKey);
        auto* pContractEntry = contractIt.tryGet();

        if (!pContractEntry) {
            return Failure_SuperContract_v2_Contract_Does_Not_Exist;
        }

        if (pContractEntry->deploymentStatus() == state::DeploymentStatus::IN_PROGRESS) {
            return Failure_SuperContract_v2_Deployment_In_Progress;
        }

        std::set<UnresolvedMosaicId> bannedMosaicIds({
        		GetUnresolvedStorageMosaicId(context.Config.Immutable),
        		config::GetUnresolvedStreamingMosaicId(context.Config.Immutable),
                config::GetUnresolvedSuperContractMosaicId(context.Config.Immutable),
                config::GetUnresolvedReviewMosaicId(context.Config.Immutable),
        });

		for (const auto& servicePayment: notification.ServicePayments) {
            if (bannedMosaicIds.count(servicePayment.MosaicId)) {
				return Failure_SuperContract_v2_Invalid_Service_Payment_Mosaic;
			}
		}

        return ValidationResult::Success;
    })

}}
