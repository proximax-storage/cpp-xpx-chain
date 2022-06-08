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

namespace catapult { namespace observers {

	using Notification = model::AccountV2UpgradeNotification<1>;

	DEFINE_OBSERVER(UpgradeCosignatory, Notification, [](const Notification& notification, const ObserverContext& context) {
		auto& multisigCache = context.Cache.sub<cache::MultisigCache>();
		if(observers::NotifyMode::Commit == context.Mode)
		{
			if(!multisigCache.contains(notification.Signer))
				return;
			auto entry = multisigCache.find(notification.Signer).get();
			for(auto account : entry.multisigAccounts())
			{
				auto mSig = multisigCache.find(account).get();
				mSig.cosignatories().erase(notification.Signer);
				mSig.cosignatories().insert(notification.NewAccountPublicKey);
			}
		}
		else
		{
			if(!multisigCache.contains(notification.NewAccountPublicKey))
				return;
			auto entry = multisigCache.find(notification.NewAccountPublicKey).get();
			for(auto account : entry.multisigAccounts())
			{
				auto mSig = multisigCache.find(account).get();
				mSig.cosignatories().erase(notification.NewAccountPublicKey);
				mSig.cosignatories().insert(notification.Signer);
			}
		}
	});
}}
