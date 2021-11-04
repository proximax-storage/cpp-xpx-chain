/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache_core/AccountStateCache.h"
#include "Validators.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/utils/CacheUtils.h"
#include "catapult/model/Address.h"
#include "catapult/state/AccountState.h"
namespace catapult { namespace validators {

	using Notification = model::AccountV2UpgradeNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(AccountV2Upgrader, [](const auto& notification, const ValidatorContext& context) {
		if(context.Config.Network.AccountVersion < 2)
			return Failure_BlockchainUpgrade_Account_Version_Not_Allowed;

		const auto& cache = context.Cache.template sub<cache::AccountStateCache>();
		auto accountRef = utils::FindAccountStateByPublicKeyOrAddress(cache, notification.Signer);
		if(!accountRef)
			return Failure_BlockchainUpgrade_Account_Non_Existant;
		auto account = accountRef->get();
		if(account.GetVersion() != 1 || state::IsRemote(account.AccountType))
			return Failure_BlockchainUpgrade_Account_Not_Upgradable;
		accountRef = utils::FindAccountStateByPublicKeyOrAddress(cache, notification.NewAccountPublicKey);
	  	if(accountRef)
			return Failure_BlockchainUpgrade_Account_Duplicate;
		return ValidationResult::Success;
	});
}}
