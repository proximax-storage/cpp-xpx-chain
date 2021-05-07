/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"

namespace catapult { namespace validators {

	using Notification = model::DataModificationCancelNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(
			DataModificationCancel,
			[](const model::DataModificationCancelNotification<1>& notification, const ValidatorContext& context) {
				const auto& driveCache = context.Cache.sub<cache::DriveCache>();
				auto driveIter = driveCache.find(notification.DriveKey);
				const auto& driveEntry = driveIter.get();

				auto dataModificationIter = std::find_if(
						driveEntry.dataModificationQueue().begin(),
						driveEntry.dataModificationQueue().end(),
						[&notification](const auto& element) { return element.first == notification.ModificationTrx; });

				if (dataModificationIter == driveEntry.dataModificationQueue().end())
					return Failure_Storage_Data_Modification_Not_Fount;

				if (dataModificationIter->second == state::DataModificationState::Active)
					return Failure_Storage_Data_Modification_Is_Active;

				return ValidationResult::Success;
			})
}}
