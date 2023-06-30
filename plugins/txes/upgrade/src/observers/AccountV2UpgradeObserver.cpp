/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/BlockchainUpgradeCache.h"
#include "catapult/model/Address.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/AccountStateCacheUtils.h"
#include "catapult/state/AccountBalances.h"
namespace catapult { namespace observers {


	using Notification = model::AccountV2UpgradeNotification<1>;


namespace {
	///Move supplemental account keys
	void TransferSupplementalKeys(state::AccountState& originalAccount, state::AccountState& newAccount)
	{
		newAccount.SupplementalPublicKeys.linked().set(originalAccount.SupplementalPublicKeys.linked().get());
		originalAccount.SupplementalPublicKeys.linked().unset();

		newAccount.SupplementalPublicKeys.node().set(originalAccount.SupplementalPublicKeys.node().get());
		originalAccount.SupplementalPublicKeys.node().unset();

		newAccount.SupplementalPublicKeys.vrf().set(originalAccount.SupplementalPublicKeys.vrf().get());
		originalAccount.SupplementalPublicKeys.vrf().unset();

		originalAccount.SupplementalPublicKeys.upgrade().set(newAccount.PublicKey);
	}
	void TransferGenerateNewState(state::AccountState& originalAccount, state::AccountState& newAccount, Height eventHeight)
	{
		TransferSupplementalKeys(originalAccount, newAccount);
		//Move balances
		// Retain original account heights
		newAccount.PublicKeyHeight = originalAccount.PublicKeyHeight;
		newAccount.AddressHeight = originalAccount.AddressHeight;

		newAccount.Balances = state::AccountBalances(&newAccount);
		newAccount.Balances.steal(originalAccount.Balances, newAccount);
		newAccount.AccountType = originalAccount.AccountType;

		originalAccount.AccountType = state::AccountType::Locked;
	}

	void ExecuteObservation(const Notification& notification, const ObserverContext& context) {
		/// We will move the entire state of account V1 to the new target account.

		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();

		if(NotifyMode::Commit == context.Mode)
		{

			auto& originalAccount = *cache::GetAccountStateByPublicKeyOrAddress(accountStateCache, notification.Signer);
			state::AccountState newAccount(model::PublicKeyToAddress(notification.NewAccountPublicKey, accountStateCache.networkIdentifier()), context.Height, 2, originalAccount);
			newAccount.PublicKey = notification.NewAccountPublicKey;
			TransferGenerateNewState(originalAccount, newAccount, context.Height);
			accountStateCache.addAccount(newAccount);
		}
		else
		{
			auto& newAccount = *cache::GetAccountStateByPublicKeyOrAddress(accountStateCache, notification.NewAccountPublicKey);
			auto& originalAccount = *cache::GetAccountStateByPublicKeyOrAddress(accountStateCache, notification.Signer);
			originalAccount = *newAccount.OldState;
			accountStateCache.queueRemove(notification.NewAccountPublicKey, newAccount.PublicKeyHeight);
			accountStateCache.commitRemovals();

		}

	}
}


	DECLARE_OBSERVER(AccountV2Upgrade, Notification)() {
		return MAKE_OBSERVER(AccountV2Upgrade, Notification, ExecuteObservation);
	}
}}
