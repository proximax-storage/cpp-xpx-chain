/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/BcDriveCache.h"

namespace catapult { namespace validators {

    using Notification = model::DataModificationCancelNotification<1>;

    DEFINE_STATEFUL_VALIDATOR(DataModificationCancel, [](const Notification& notification, const ValidatorContext& context) {
        const auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
        const auto driveIter = driveCache.find(notification.DriveKey);
        const auto& pDriveEntry = driveIter.tryGet();
        if (!pDriveEntry)
          return Failure_Storage_Drive_Not_Found;
        const auto& activeDataModifications = pDriveEntry->activeDataModifications();

        if (notification.Owner != pDriveEntry->owner())
          return Failure_Storage_Is_Not_Owner;

        if (activeDataModifications.empty())
          return Failure_Storage_No_Active_Data_Modifications;

        auto dataModificationIter = activeDataModifications.begin();
        for (; dataModificationIter !=
             activeDataModifications.end(); ++dataModificationIter) {
          if (dataModificationIter->Id == notification.DataModificationId)
              break;
        }

        if (dataModificationIter == activeDataModifications.end())
          return Failure_Storage_Data_Modification_Not_Found;

        return ValidationResult::Success;
    })
}}
