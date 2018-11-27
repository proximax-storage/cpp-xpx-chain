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

#include "Validators.h"
#include "src/cache/MultisigCache.h"
#include "catapult/utils/Functional.h"
#include "catapult/utils/Hashers.h"
#include "catapult/utils/RawBuffer.h"
#include "catapult/validators/ValidatorContext.h"
#include <limits>

namespace catapult { namespace validators {

	using Notification = model::ModifyMultisigSettingsNotification;

	DEFINE_STATEFUL_VALIDATOR(ModifyMultisigInvalidSettings, [](const auto& notification, const ValidatorContext& context) {
		const auto& multisigCache = context.Cache.sub<cache::MultisigCache>();
		if (!multisigCache.contains(notification.Signer)) {
			// since the ModifyMultisigInvalidCosignersValidator and the ModifyMultisigCosignersObserver ran before
			// this validator, the only scenario in which the multisig account cannot be found in the multisig cache
			// is that the observer removed the last cosigner reverting the multisig account to a normal accounts

			// MinRemovalDelta and MinApprovalDelta are both expected to be -1 in this case
			if (-1 != notification.MinRemovalDelta || -1 != notification.MinApprovalDelta)
				return Failure_Multisig_Modify_Min_Setting_Out_Of_Range;

			return ValidationResult::Success;
		}

		auto multisigIter = multisigCache.find(notification.Signer);
		const auto& multisigEntry = multisigIter.get();
		int newMinRemoval = multisigEntry.minRemoval() + notification.MinRemovalDelta;
		int newMinApproval = multisigEntry.minApproval() + notification.MinApprovalDelta;
		if (1 > newMinRemoval || 1 > newMinApproval)
			return Failure_Multisig_Modify_Min_Setting_Out_Of_Range;

		int maxValue = static_cast<int>(multisigEntry.cosignatories().size());
		if (newMinRemoval > maxValue || newMinApproval > maxValue)
			return Failure_Multisig_Modify_Min_Setting_Larger_Than_Num_Cosignatories;

		return ValidationResult::Success;
	});
}}
