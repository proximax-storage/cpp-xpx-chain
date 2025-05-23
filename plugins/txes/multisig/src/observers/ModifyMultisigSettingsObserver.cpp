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

	using Notification = model::ModifyMultisigSettingsNotification<1>;

	namespace {
		constexpr uint8_t AddDelta(uint8_t value, int8_t delta) {
			return value + static_cast<uint8_t>(delta);
		}
	}

	DEFINE_OBSERVER(ModifyMultisigSettings, Notification, [](const Notification& notification, const ObserverContext& context) {
		auto& multisigCache = context.Cache.sub<cache::MultisigCache>();
		auto isNotContained = !multisigCache.contains(notification.Signer);
		if (isNotContained && observers::NotifyMode::Commit == context.Mode)
			return;

		if (isNotContained && observers::NotifyMode::Rollback == context.Mode && notification.ModificationsCount == 0)
			return;

		// note that in case of a rollback the multisig entry needs to be restored to the original state, else the multisig settings
		// validator will reject the (invalid) min approval / removal
		if (isNotContained)
			multisigCache.insert(state::MultisigEntry(notification.Signer));

		auto multisigIter = multisigCache.find(notification.Signer);
		auto& multisigEntry = multisigIter.get();

		int8_t direction = NotifyMode::Commit == context.Mode ? 1 : -1;
		multisigEntry.setMinApproval(AddDelta(multisigEntry.minApproval(), direction * notification.MinApprovalDelta));
		multisigEntry.setMinRemoval(AddDelta(multisigEntry.minRemoval(), direction * notification.MinRemovalDelta));
	});
}}
