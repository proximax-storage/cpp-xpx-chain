/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonDrive.h"
#include <cmath>

namespace catapult { namespace observers {

    namespace {
        inline uint64_t Time(const state::DriveEntry& entry, const Key& replicator) {
            const auto& last = entry.billingHistory().back();
            return last.End.unwrap() - std::max(last.Start.unwrap(), entry.replicators().at(replicator).Start.unwrap());
        }
    }

    void Transfer(state::AccountState& debitState, state::AccountState& creditState, MosaicId mosaicId, Amount amount, Height height) {
        debitState.Balances.debit(mosaicId, amount, height);
        creditState.Balances.credit(mosaicId, amount, height);
    }

    void DrivePayment(state::DriveEntry& driveEntry, const ObserverContext& context, const MosaicId& storageMosaicId) {
        if (driveEntry.billingHistory().empty() || driveEntry.billingHistory().back().End != context.Height)
            return;

        auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
        auto accountIter = accountStateCache.find(driveEntry.key());
        auto& driveAccount = accountIter.get();

        if (NotifyMode::Commit == context.Mode) {
            auto sum = utils::GetBillingBalanceOfDrive(driveEntry, context.Cache, storageMosaicId).unwrap();
            uint64_t sumTime = 0;
            uint64_t remains = sum;

            const auto& replicators = driveEntry.replicators();

            for (const auto& replicatorPair : replicators) {
                sumTime += Time(driveEntry, replicatorPair.first);
            }

            auto i = 0u;
            for (const auto& replicatorPair : replicators) {
                uint64_t time = Time(driveEntry, replicatorPair.first);
                uint64_t reward = std::floor(double(sum) * time / sumTime);

                // The last replicator takes remaining tokens, it is need to resolve the integer division
                if (i == replicators.size() - 1)
                    reward = remains;
                remains -= reward;

                auto replicatorIter = accountStateCache.find(replicatorPair.first);
                auto& replicatorAccount = replicatorIter.get();
                Transfer(driveAccount, replicatorAccount, storageMosaicId, Amount(reward), context.Height);

                driveEntry.billingHistory().back().Payments.emplace_back(state::PaymentInformation{
                    replicatorPair.first,
                    Amount(reward),
                    context.Height
                });

                ++i;
            }
        } else {
            auto& last = driveEntry.billingHistory().back();
            for (const auto& payment : last.Payments) {
                auto replicatorIter = accountStateCache.find(payment.Receiver);
                auto& replicatorAccount = replicatorIter.get();
                Transfer(replicatorAccount, driveAccount, storageMosaicId, payment.Amount, context.Height);

                if (payment.Height != context.Height)
                    CATAPULT_THROW_RUNTIME_ERROR("Got unexpected state during rollback of billing end");
            }

            last.Payments.clear();
        }
    }

}}
