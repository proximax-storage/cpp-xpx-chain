/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonDrive.h"
#include "plugins/txes/multisig/src/observers/MultisigAccountFacade.h"
#include <cmath>

namespace catapult { namespace observers {

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
}}
