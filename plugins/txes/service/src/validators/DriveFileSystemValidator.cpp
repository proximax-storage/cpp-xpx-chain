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
		auto driveIter = driveCache.find(notification.DriveKey);
		const auto& driveEntry = driveIter.get();

        if (!driveEntry.isOwner(notification.Signer))
            return Failure_Service_Operation_Is_Not_Permitted;

		if (notification.XorRootHash == Hash256())
			return Failure_Service_Drive_Root_No_Changes;

		if ((notification.XorRootHash ^ notification.RootHash) != driveEntry.rootHash())
			return Failure_Service_Root_Hash_Is_Not_Equal;

		utils::HashSet hashes;
		auto addActionsPtr = notification.AddActionsPtr;
		for (auto i = 0u; i < notification.AddActionsCount; ++i, ++addActionsPtr) {
			hashes.insert(addActionsPtr->FileHash);
            if (driveEntry.files().count(addActionsPtr->FileHash)) {
            	return Failure_Service_File_Hash_Redundant;
            }
		}

		if (hashes.size() != notification.AddActionsCount)
			return Failure_Service_File_Hash_Redundant;

		hashes.clear();
		auto removeActionsPtr = notification.RemoveActionsPtr;
		for (auto i = 0u; i < notification.RemoveActionsCount; ++i, ++removeActionsPtr) {
			hashes.insert(removeActionsPtr->FileHash);
            if (!driveEntry.files().count(removeActionsPtr->FileHash)) {
            	return Failure_Service_File_Doesnt_Exist;
            }
			if (driveEntry.files().at(removeActionsPtr->FileHash).Size != removeActionsPtr->FileSize) {
				return Failure_Service_Remove_Files_Not_Same_File_Size;
			}
		}

		if (hashes.size() != notification.RemoveActionsCount)
			return Failure_Service_File_Hash_Redundant;

		return ValidationResult::Success;
	});
}}
