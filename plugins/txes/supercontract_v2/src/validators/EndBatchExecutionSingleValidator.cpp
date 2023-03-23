/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

    using Notification = model::BatchExecutionSingleNotification<1>;

    DEFINE_STATEFUL_VALIDATOR(EndBatchExecutionSingle, [](const Notification& notification, const ValidatorContext& context) {

        const auto& contractCache = context.Cache.sub<cache::SuperContractCache>();

        auto contractIt = contractCache.find(notification.ContractKey);
        const auto& contractEntry = contractIt.get();

        if (notification.BatchId + 1 != contractEntry.nextBatchId()) {
            return Failure_SuperContract_v2_Invalid_Batch_Id;
        }

        return ValidationResult::Success;
    })

}}
