/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonDrive.h"
#include "plugins/txes/multisig/src/observers/MultisigAccountFacade.h"	// TODO: Dependency on another plugin?
#include <cmath>

namespace catapult { namespace observers {

    namespace {
        inline uint64_t Time(const state::DriveEntry& entry, const Key& replicatorKey) {
            const auto& last = entry.billingHistory().back();
            const auto& replicator = entry.replicators().at(replicatorKey);
            auto start = std::max(last.Start.unwrap(), replicator.Start.unwrap());
            auto end = (Height(0) == replicator.End) ? last.End.unwrap() : std::min(last.End.unwrap(), replicator.End.unwrap());
            if (end < start)
            	CATAPULT_THROW_RUNTIME_ERROR_2("invalid replicator time", start, end);
            return std::max(end - start, static_cast<uint64_t>(1));
        }
    }

    void Transfer(state::AccountState& debitState, state::AccountState& creditState, MosaicId mosaicId, Amount amount, ObserverContext& context) {
        debitState.Balances.debit(mosaicId, amount, context.Height);
        creditState.Balances.credit(mosaicId, amount, context.Height);

        if (observers::NotifyMode::Commit == context.Mode) {
            model::BalanceChangeReceipt receiptCredit(model::Receipt_Type_Drive_Reward_Transfer_Credit, creditState.PublicKey, mosaicId, amount);
            context.StatementBuilder().addTransactionReceipt(receiptCredit);

            model::BalanceChangeReceipt receiptDebit(model::Receipt_Type_Drive_Reward_Transfer_Debit, debitState.PublicKey, mosaicId, amount);
            context.StatementBuilder().addTransactionReceipt(receiptDebit);
        }
    }

    void Credit(state::AccountState& creditState, MosaicId mosaicId, Amount amount, ObserverContext& context) {
        creditState.Balances.credit(mosaicId, amount, context.Height);

        if (observers::NotifyMode::Commit == context.Mode) {
            model::BalanceChangeReceipt receipt(model::Receipt_Type_Drive_Deposit_Credit, creditState.PublicKey, mosaicId, amount);
            context.StatementBuilder().addTransactionReceipt(receipt);
        }
    }

    void Debit(state::AccountState& debitState, MosaicId mosaicId, Amount amount, ObserverContext& context) {
        debitState.Balances.debit(mosaicId, amount, context.Height);

        if (observers::NotifyMode::Commit == context.Mode) {
            model::BalanceChangeReceipt receipt(model::Receipt_Type_Drive_Deposit_Debit, debitState.PublicKey, mosaicId, amount);
            context.StatementBuilder().addTransactionReceipt(receipt);
        }
    }

    void DrivePayment(state::DriveEntry& driveEntry, ObserverContext& context, const MosaicId& storageMosaicId, std::vector<Key> replicators) {
        if (driveEntry.billingHistory().empty() || (replicators.empty() && driveEntry.billingHistory().back().End != context.Height))
            return;

        auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
        auto accountIter = accountStateCache.find(driveEntry.key());
        auto& driveAccount = accountIter.get();

        if (NotifyMode::Commit == context.Mode) {
            auto sum = utils::GetDriveBalance(driveEntry, context.Cache, storageMosaicId).unwrap();
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
                uint64_t reward = std::floor(double(sum) * time / sumTime);

                // In the case of full payment for the billing period the last replicator takes remaining tokens.
                // This is required to resolve rounding errors.
                if (fullPayment && (i == driveEntry.replicators().size() - 1))
                    reward = remains;
                remains -= reward;

                auto replicatorIter = accountStateCache.find(replicator);
                auto& replicatorAccount = replicatorIter.get();
                Transfer(driveAccount, replicatorAccount, storageMosaicId, Amount(reward), context);

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
                Transfer(replicatorAccount, driveAccount, storageMosaicId, payment.Amount, context);

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
		auto multisigEntry = multisigIter.tryGet();
		if (!multisigEntry)
		    return;

		float cosignatoryCount = driveEntry.replicators().size();
		uint8_t minCosignatory = ceil(cosignatoryCount * driveEntry.percentApprovers() / 100);
		multisigEntry->setMinApproval(minCosignatory);
		multisigEntry->setMinRemoval(minCosignatory);
	}

	void RemoveDriveMultisig(state::DriveEntry& driveEntry, observers::ObserverContext& context) {
		auto& multisigCache = context.Cache.sub<cache::MultisigCache>();
    	observers::MultisigAccountFacade facade(multisigCache, driveEntry.key());

    	for (const auto& replicatorPair : driveEntry.replicators()) {
    		if (observers::NotifyMode::Commit == context.Mode) {
			    facade.removeCosignatory(replicatorPair.first);
    		} else {
			    facade.addCosignatory(replicatorPair.first);
    		}
    	}

		if (observers::NotifyMode::Rollback == context.Mode) {
			UpdateDriveMultisigSettings(driveEntry, context);
		}
	}
}}
