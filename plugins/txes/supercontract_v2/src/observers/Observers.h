/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/observers/ObserverTypes.h"
#include "catapult/model/SupercontractNotifications.h"
#include "src/cache/SuperContractCache.h"
#include "src/cache/DriveContractCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "src/model/InternalSuperContractNotifications.h"
#include <catapult/observers/LiquidityProviderExchangeObserver.h>
#include <catapult/observers/StorageExternalManagementObserver.h>
#include <queue>
#include <catapult/state/DriveStateBrowser.h>

namespace catapult::observers {

	DECLARE_OBSERVER(DeployContract, model::DeploySupercontractNotification<1>)();

	DECLARE_OBSERVER(AutomaticExecutionsReplenishment, model::AutomaticExecutionsReplenishmentNotification<1>)();

	DECLARE_OBSERVER(ManualCall, model::ManualCallNotification<1>)();

	DECLARE_OBSERVER(ProofOfExecution, model::ProofOfExecutionNotification<1>)(
			const std::unique_ptr<LiquidityProviderExchangeObserver>&,
			const std::unique_ptr<StorageExternalManagementObserver>&);

	DECLARE_OBSERVER(BatchCalls, model::BatchCallsNotification<1>)(
			const std::unique_ptr<LiquidityProviderExchangeObserver>&,
			const std::unique_ptr<state::DriveStateBrowser>&);

	DECLARE_OBSERVER(ContractStateUpdate, model::ContractStateUpdateNotification<1>)(
			const std::unique_ptr<state::DriveStateBrowser>&);

	DECLARE_OBSERVER(ContractDestroy, model::ContractDestroyNotification<1>)(
			const std::unique_ptr<observers::StorageExternalManagementObserver>&);

	DECLARE_OBSERVER(EndBatchExecution, model::EndBatchExecutionNotification<1>)(
			const std::unique_ptr<state::DriveStateBrowser>&);

	DECLARE_OBSERVER(SuccessfulEndBatchExecution, model::SuccessfulBatchExecutionNotification<1>)(
			const std::unique_ptr<StorageExternalManagementObserver>&);

	DECLARE_OBSERVER(UnsuccessfulEndBatchExecution, model::UnsuccessfulBatchExecutionNotification<1>)();

	DECLARE_OBSERVER(SynchronizationSingle, model::SynchronizationSingleNotification<1>)(
			const std::unique_ptr<StorageExternalManagementObserver>&);

	DECLARE_OBSERVER(ReleasedTransactions, model::ReleasedTransactionsNotification<1>)();
}
