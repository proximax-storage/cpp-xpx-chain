/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"

namespace catapult { namespace validators {

	using Notification = model::FilesDepositNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(FilesDeposit, [](const Notification& notification, const StatefulValidatorContext& context) {
		const auto& driveCache = context.Cache.sub<cache::DriveCache>();
		auto driveIter = driveCache.find(notification.DriveKey);
		const auto& driveEntry = driveIter.get();

		if (!driveEntry.replicators().count(notification.Replicator))
            return Failure_Service_Drive_Replicator_Not_Registered;

		const auto& activeFilesWithoutDeposit = driveEntry.replicators().at(notification.Replicator).ActiveFilesWithoutDeposit;

		({
			std::set<Hash256> hashes;
			auto filesPtr = notification.FilesPtr;
			for (auto i = 0u; i < notification.FilesCount; ++i, ++filesPtr) {
				hashes.insert(filesPtr->FileHash);
				if (!activeFilesWithoutDeposit.count(filesPtr->FileHash))
					return Failure_Service_File_Doesnt_Exist;
			}

            if (hashes.size() != notification.FilesCount)
                return Failure_Service_File_Hash_Redundant;
		});

		return ValidationResult::Success;
	});
}}
