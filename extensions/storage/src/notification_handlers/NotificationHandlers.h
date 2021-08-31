/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/StorageNotifications.h"
#include "catapult/notification_handlers/HandlerContext.h"
#include "catapult/notification_handlers/NotificationHandlerTypes.h"
#include "../ReplicatorService.h"

namespace catapult { namespace notification_handlers {

	DECLARE_HANDLER(PrepareDrive, model::PrepareDriveNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(DataModification, model::DataModificationNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(DataModificationCancel, model::DataModificationCancelNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(Download, model::DownloadNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(ReplicatorOnboarding, model::ReplicatorOnboardingNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(DriveClosure, model::DriveClosureNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);
}}
