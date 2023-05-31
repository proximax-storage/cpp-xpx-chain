/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "Observers.h"
#include "src/cache/MultisigCache.h"
#include "MultisigAccountFacade.h"

namespace catapult { namespace observers {

	using Notification = model::AccountV2UpgradeNotification<1>;

	namespace {
		void Observe(const Notification& notification, const ObserverContext& context) {
			auto& multisigCache = context.Cache.sub<cache::MultisigCache>();
			if(observers::NotifyMode::Commit == context.Mode)
			{
				/// Verify if account is multisig or cosigns for multisig
				if(!multisigCache.contains(notification.Signer))
					return;

				const auto entry = multisigCache.find(notification.Signer).get();
				/// Check if there are multisig accounts that this account cosigns for
				for(auto account : entry.multisigAccounts())
				{
					MultisigAccountFacade multisigAccountFacade(multisigCache, account);
					multisigAccountFacade.removeCosignatory(notification.Signer);
					multisigAccountFacade.addCosignatory(notification.NewAccountPublicKey);
				}
				/// If account is multisig we transfer the cosignatories.
				if(!entry.cosignatories().empty()) {
					MultisigAccountFacade multisigAccountFacadeOld(multisigCache, notification.Signer);
					MultisigAccountFacade multisigAccountFacadeNew(multisigCache, notification.NewAccountPublicKey);
					for(auto account : entry.cosignatories())
					{
						multisigAccountFacadeOld.removeCosignatory(account);
						multisigAccountFacadeNew.addCosignatory(account);
					}
				}
			}
			else
			{
				if(!multisigCache.contains(notification.NewAccountPublicKey))
					return;

				const auto entry = multisigCache.find(notification.NewAccountPublicKey).get();
				/// Check if there are multisig accounts that this account cosigns for
				for(auto account : entry.multisigAccounts())
				{
					MultisigAccountFacade multisigAccountFacade(multisigCache, account);
					multisigAccountFacade.removeCosignatory(notification.NewAccountPublicKey);
					multisigAccountFacade.addCosignatory(notification.Signer);
				}
				/// If account is multisig we transfer the cosignatories.
				if(!entry.cosignatories().empty()) {
					MultisigAccountFacade multisigAccountFacadeOld(multisigCache, notification.NewAccountPublicKey);
					MultisigAccountFacade multisigAccountFacadeNew(multisigCache, notification.Signer);
					for(auto account : entry.cosignatories())
					{
						multisigAccountFacadeOld.removeCosignatory(account);
						multisigAccountFacadeNew.addCosignatory(account);
					}
				}
			}
		}
	}
	DEFINE_OBSERVER(UpgradeMultisig, Notification, Observe);
}}
