/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "src/config/ContractConfiguration.h"
#include "src/model/ContractNotifications.h"
#include "catapult/observers/ObserverTypes.h"

namespace catapult { namespace observers {

	/// Observes changes triggered by update contract notifications
	DECLARE_OBSERVER(ModifyContract, model::ModifyContractNotification<1>)();

	/// Observes changes triggered by update reputation notifications
	DECLARE_OBSERVER(ReputationUpdate, model::ReputationUpdateNotification<1>)();

	/// Observes clean up contract, account and multisig cache, when balance of multisig account is zero
	DECLARE_OBSERVER(CleanUpContract, model::BalanceTransferNotification<1>)();
}}
