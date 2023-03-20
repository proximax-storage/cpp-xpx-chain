/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/SuperContractCache.h"
#include "src/cache/DriveContractCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "SuperContractStorageUpdatesListener.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"

namespace catapult::observers {

SuperContractStorageUpdatesListener::SuperContractStorageUpdatesListener(
		const std::unique_ptr<state::DriveStateBrowser>& driveStateBrowser,
		const std::unique_ptr<observers::LiquidityProviderExchangeObserver>& liquidityProvider)
	: m_driveStateBrowser(driveStateBrowser), m_liquidityProvider(liquidityProvider) {}

    void SuperContractStorageUpdatesListener::onDriveClosed(
            catapult::observers::ObserverContext& context,
            const catapult::Key& driveKey) const {
        auto& driveContractCache = context.Cache.sub<cache::DriveContractCache>();
        auto driveIt = driveContractCache.find(driveKey);
        auto pDriveEntry = driveIt.tryGet();
        if (!pDriveEntry) {
            return;
        }

        auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
        auto contractIt = contractCache.find(pDriveEntry->contractKey());
        auto& contractEntry = contractIt.get();

        auto& accountCache = context.Cache.sub<cache::AccountStateCache>();

        auto contractAccountIt = accountCache.find(contractEntry.key());
        auto& contractAccountEntry = contractAccountIt.get();

        auto scMosaicId = config::GetUnresolvedSuperContractMosaicId(context.Config.Immutable);
        auto streamingMosaicId = config::GetUnresolvedStreamingMosaicId(context.Config.Immutable);

        for (const auto& call : contractEntry.requestedCalls()) {
            auto callerAccountIt = accountCache.find(call.Caller);
            auto& callerAccountEntry = callerAccountIt.get();
            for (const auto& [mosaicId, amount] : call.ServicePayments) {
                auto resolvedMosaicId = context.Resolvers.resolve(mosaicId);
                auto balance = contractAccountEntry.Balances.get(resolvedMosaicId);
                if (balance >= amount) {
                    contractAccountEntry.Balances.debit(resolvedMosaicId, amount);
                    callerAccountEntry.Balances.credit(resolvedMosaicId, amount);
                }
            }
            auto orderedExecutors =
                    m_driveStateBrowser->getOrderedReplicatorsCount(context.Cache.toReadOnly(), driveKey);
            Amount executionAmount(call.ExecutionCallPayment.unwrap() * orderedExecutors);
            Amount downloadAmount(call.DownloadCallPayment.unwrap() * orderedExecutors);
            m_liquidityProvider->debitMosaics(
                    context, contractEntry.executionPaymentKey(), call.Caller, scMosaicId, executionAmount);
            m_liquidityProvider->debitMosaics(
                    context, contractEntry.executionPaymentKey(), call.Caller, streamingMosaicId, downloadAmount);
        }

        auto assigneeAccountIt = accountCache.find(contractEntry.assignee());
        auto& assigneeAccountEntry = assigneeAccountIt.get();
        for (const auto& [mosaicId, amount] : contractAccountEntry.Balances) {
            contractAccountEntry.Balances.debit(mosaicId, amount);
            assigneeAccountEntry.Balances.credit(mosaicId, amount);
        }

        auto contractExecutionAccountIt = accountCache.find(contractEntry.executionPaymentKey());
        auto& contractExecutionAccountEntry = contractExecutionAccountIt.get();
        for (const auto& [mosaicId, amount] : contractExecutionAccountEntry.Balances) {
            contractExecutionAccountEntry.Balances.debit(mosaicId, amount);
            assigneeAccountEntry.Balances.credit(mosaicId, amount);
        }

        driveContractCache.remove(driveKey);
        contractCache.remove(contractEntry.key());
    }
}