/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"
#include "src/cache/SuperContractCache.h"
#include "src/model/SuperContractEntityType.h"

namespace catapult { namespace validators {

	using Notification = model::DriveFileSystemNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DriveFileSystem, [](const Notification& notification, const ValidatorContext& context) {
		if (!notification.RemoveActionsCount)
			return ValidationResult::Success;

		const auto& superContractCache = context.Cache.sub<cache::SuperContractCache>();

		const auto& driveCache = context.Cache.sub<cache::DriveCache>();
		auto driveIter = driveCache.find(notification.DriveKey);
		const state::DriveEntry& driveEntry = driveIter.get();

		for (const auto& coowner : driveEntry.coowners()) {
			if (superContractCache.contains(coowner)) {
				auto contractIter = superContractCache.find(coowner);
				const state::SuperContractEntry& contractEntry = contractIter.get();

				if (contractEntry.state() == state::SuperContractState::Active) {
					auto pRemove = notification.RemoveActionsPtr;
					for (auto i = 0u; i < notification.RemoveActionsCount; ++i, ++pRemove) {
						if (pRemove->FileHash == contractEntry.fileHash()) {
							return Failure_SuperContract_Remove_Super_Contract_File;
						}
					}
				}
			}
		}

		return ValidationResult::Success;
	});
}}
