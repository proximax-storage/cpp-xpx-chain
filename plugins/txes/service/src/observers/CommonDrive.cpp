/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonDrive.h"
#include "plugins/txes/multisig/src/cache/MultisigCache.h"
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

    void DrivePayment(state::DriveEntry& driveEntry, const ObserverContext& context, const MosaicId& storageMosaicId, std::vector<Key> replicators) {
        if (driveEntry.billingHistory().empty() || (replicators.empty() && driveEntry.billingHistory().back().End != context.Height))
            return;

        auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
        auto accountIter = accountStateCache.find(driveEntry.key());
        auto& driveAccount = accountIter.get();

        if (NotifyMode::Commit == context.Mode) {
            auto sum = utils::GetBillingBalanceOfDrive(driveEntry, context.Cache, storageMosaicId).unwrap();
            uint64_t sumTime = 0;
            uint64_t remains = sum;

            for (const auto& replicatorPair : driveEntry.replicators()) {
                sumTime += Time(driveEntry, replicatorPair.first);
            }

            bool fullPayment = replicators.empty() || replicators.size() == driveEntry.replicators().size();
            if (replicators.empty() ) {
				replicators.reserve(driveEntry.replicators().size());
				for (const auto& replicatorPair : driveEntry.replicators()) {
					replicators.emplace_back(replicatorPair.first);
				}
			}

            auto i = 0u;
            for (const auto& replicator : replicators) {
                uint64_t time = Time(driveEntry, replicator);
                auto share = double(sum) * time / sumTime;
                uint64_t reward = (i % 2) ? std::ceil(share) : std::floor(share);

                // In the case of full payment for the billing period the last replicator takes remaining tokens.
                // This is required to resolve rounding errors.
                if (fullPayment && (i == driveEntry.replicators().size() - 1))
                    reward = remains;
                remains -= reward;

                auto replicatorIter = accountStateCache.find(replicator);
                auto& replicatorAccount = replicatorIter.get();
                Transfer(driveAccount, replicatorAccount, storageMosaicId, Amount(reward), context.Height);

                driveEntry.billingHistory().back().Payments.emplace_back(state::PaymentInformation{
                        replicator,
                        Amount(reward),
                        context.Height
                });

                ++i;
            }
        } else {
			if (driveEntry.billingHistory().empty())
				CATAPULT_THROW_RUNTIME_ERROR("unexpected billing history during rollback");

            auto& payments = driveEntry.billingHistory().back().Payments;
			if (payments.empty() || payments.back().Height != context.Height)
				CATAPULT_THROW_RUNTIME_ERROR("unexpected payments during rollback");

            while (!payments.empty()) {
            	const auto& payment = payments.back();
				if (payment.Height != context.Height)
					break;

                auto replicatorIter = accountStateCache.find(payment.Receiver);
                auto& replicatorAccount = replicatorIter.get();
                Transfer(replicatorAccount, driveAccount, storageMosaicId, payment.Amount, context.Height);

				payments.pop_back();
            }
        }
    }

	void SetDriveState(state::DriveEntry& entry, observers::ObserverContext& context, state::DriveState driveState) {
		if (entry.state() == driveState)
			return;

		if (observers::NotifyMode::Commit == context.Mode)
			context.StatementBuilder().addPublicKeyReceipt(model::DriveStateReceipt(model::Receipt_Type_Drive_State, entry.key(), utils::to_underlying_type(driveState)));
		entry.setState(driveState);
	}

	void UpdateDriveMultisigSettings(state::DriveEntry& driveEntry, observers::ObserverContext& context) {
		auto& multisigCache = context.Cache.sub<cache::MultisigCache>();
		auto multisigIter = multisigCache.find(driveEntry.key());
		auto& multisigEntry = multisigIter.get();
		float cosignatoryCount = driveEntry.replicators().size();
		multisigEntry.setMinApproval(ceil(cosignatoryCount * driveEntry.percentApprovers() / 100));
		multisigEntry.setMinRemoval(ceil(cosignatoryCount * driveEntry.percentApprovers() / 100));
	}
}}
