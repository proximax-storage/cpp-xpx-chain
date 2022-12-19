/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"
#include "src/catapult/observers/LiquidityProviderExchangeObserver.h"
#include "src/catapult/cache/ReadOnlyCatapultCache.h"

namespace catapult::observers {

	using Notification = model::ContractDestroyNotification<1>;

	DECLARE_OBSERVER(ContractDestruction, Notification)(
			const std::unique_ptr<observers::StorageExternalManagementObserver>& storageExternalManager) {
		return MAKE_OBSERVER(ContractDestruction, Notification, ([&storageExternalManager](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ContractDestroy)");

			auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
			auto contractIt = contractCache.find(notification.ContractKey);
			auto& contractEntry = contractIt.get();

			auto& accountCache = context.Cache.sub<cache::AccountStateCache>();

			auto assigneeAccountIt = accountCache.find(contractEntry.assignee());
			auto& assigneeAccountEntry = assigneeAccountIt.get();

			auto contractExecutionAccountIt = accountCache.find(contractEntry.executionPaymentKey());
			auto& contractExecutionAccountEntry = contractExecutionAccountIt.get();
			for (const auto& [mosaicId, amount] : contractExecutionAccountEntry.Balances) {
				contractExecutionAccountEntry.Balances.debit(mosaicId, amount);
				assigneeAccountEntry.Balances.credit(mosaicId, amount);
			}

			auto& driveContractCache = context.Cache.sub<cache::DriveContractCache>();
			driveContractCache.remove(contractEntry.driveKey());

			storageExternalManager->allowOwnerManagement(context, contractEntry.driveKey());

			contractCache.remove(notification.ContractKey);
		}))
	}
}
