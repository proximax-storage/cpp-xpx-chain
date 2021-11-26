/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache_core/AccountStateCache.h"
#include "Validators.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/cache_core/AccountStateCacheUtils.h"
#include "catapult/model/Address.h"
#include "catapult/state/AccountState.h"
namespace catapult { namespace validators {

	using Notification = model::AccountV2UpgradeNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(AccountV2Upgrade, [](const auto& notification, const ValidatorContext& context) {
		if(context.Config.Network.AccountVersion < 2)
			return Failure_BlockchainUpgrade_Account_Version_Not_Allowed;

		const auto& cache = context.Cache.template sub<cache::AccountStateCache>();
		auto accountPtr = cache::FindAccountStateByPublicKeyOrAddress(cache, notification.Signer);
		if(!accountPtr)
			return Failure_BlockchainUpgrade_Account_Non_Existant;
		auto account = *accountPtr;
		if(account.GetVersion() != 1 || state::IsRemote(account.AccountType) || account.IsLocked())
			return Failure_BlockchainUpgrade_Account_Not_Upgradable;
		accountPtr = cache::FindAccountStateByPublicKeyOrAddress(cache, notification.NewAccountPublicKey);
	  	if(accountPtr)
			return Failure_BlockchainUpgrade_Account_Duplicate;
		return ValidationResult::Success;
	});
}}
