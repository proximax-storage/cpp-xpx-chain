/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"
#include "src/model/InternalSuperContractNotifications.h"
#include "catapult/model/SupercontractNotifications.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "src/cache/SuperContractCache.h"
#include "src/cache/DriveContractCache.h"

namespace catapult { namespace validators {

	DECLARE_STATELESS_VALIDATOR(SuperContractV2PluginConfig, model::PluginConfigNotification<1>)();

	DECLARE_STATEFUL_VALIDATOR(ManualCall, model::ManualCallNotification<1>)();

	DECLARE_STATEFUL_VALIDATOR(AutomaticExecutionsReplenishment, model::AutomaticExecutionsReplenishmentNotification<1>)();

	DECLARE_STATEFUL_VALIDATOR(DeployContract, model::DeploySupercontractNotification<1>)();

	DECLARE_STATEFUL_VALIDATOR(EndBatchExecution, model::EndBatchExecutionNotification<1>)();

	DECLARE_STATEFUL_VALIDATOR(BatchCalls, model::BatchCallsNotification<1>)();

	DECLARE_STATEFUL_VALIDATOR(ContractStateUpdate, model::ContractStateUpdateNotification<1>)();

	DECLARE_STATEFUL_VALIDATOR(OpinionSignature, model::OpinionSignatureNotification<1>)();

	DECLARE_STATEFUL_VALIDATOR(ProofOfExecution, model::ProofOfExecutionNotification<1>)();

	DECLARE_STATEFUL_VALIDATOR(EndBatchExecutionSingle, model::BatchExecutionSingleNotification<1>)();

	DECLARE_STATEFUL_VALIDATOR(SynchronizationSingle, model::SynchronizationSingleNotification<1>)();

	DECLARE_STATEFUL_VALIDATOR(ReleasedTransactions, model::ReleasedTransactionsNotification<1>)();
}}
