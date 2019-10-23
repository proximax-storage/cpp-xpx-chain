/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::DriveFileSystemNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DriveFileSystem, [](const Notification& notification, const ValidatorContext& context) {
		const auto& driveCache = context.Cache.sub<cache::DriveCache>();
		if (!driveCache.contains(notification.DriveKey))
			return Failure_Service_Drive_Does_Not_Exist;

		const auto& driveEntry = driveCache.find(notification.DriveKey).get();

        if (notification.Signer != driveEntry.owner())
            return Failure_Service_Operation_Is_Not_Permitted;

		if ((notification.XorRootHash ^ notification.RootHash) != driveEntry.rootHash())
			return Failure_Service_Root_Hash_Is_Not_Equal;

		auto addActionsPtr = notification.AddActionsPtr;
		for (auto i = 0u; i < notification.AddActionsCount; ++i, ++addActionsPtr) {
            if (driveEntry.files().count(addActionsPtr->FileHash)) {
                const auto& file = driveEntry.files().find(addActionsPtr->FileHash)->second;

                if (file.isActive())
                    return Failure_Service_File_Hash_Redudant;
            }
		}

		auto removeActionsPtr = notification.RemoveActionsPtr;
		for (auto i = 0u; i < notification.RemoveActionsCount; ++i, ++removeActionsPtr) {
            if (!driveEntry.files().count(removeActionsPtr->FileHash)) {
                return Failure_Service_File_Is_Not_Exist;
            } else {
                const auto& file = driveEntry.files().find(addActionsPtr->FileHash)->second;

                if (!file.isActive())
                    return Failure_Service_File_Is_Not_Exist;
            }
		}

		return ValidationResult::Success;
	});
}}
