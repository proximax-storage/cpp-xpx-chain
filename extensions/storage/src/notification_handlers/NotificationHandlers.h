/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/StorageNotifications.h"
#include "catapult/model/ServiceStorageNotifications.h"
#include "catapult/notification_handlers/HandlerContext.h"
#include "catapult/notification_handlers/NotificationHandlerTypes.h"
#include "../ReplicatorService.h"

namespace catapult { namespace notification_handlers {

	DECLARE_HANDLER(DataModificationApprovalService, model::DataModificationApprovalServiceNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(DataModificationCancelService, model::DataModificationCancelServiceNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(DataModificationService, model::DataModificationServiceNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(DataModificationSingleApprovalService, model::DataModificationSingleApprovalServiceNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(DownloadApprovalService, model::DownloadApprovalServiceNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(DownloadPaymentService, model::DownloadPaymentServiceNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(DownloadRewardService, model::DownloadRewardServiceNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(DownloadService, model::DownloadServiceNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(DrivesUpdateService, model::DrivesUpdateServiceNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(EndDriveVerificationService, model::EndDriveVerificationServiceNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(HealthCheck, model::BlockNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(PrepareDriveService, model::PrepareDriveServiceNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(ReplicatorOnboardingService, model::ReplicatorOnboardingServiceNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(StartDriveVerificationService, model::StartDriveVerificationServiceNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(StreamFinishService, model::StreamFinishServiceNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(StreamPaymentService, model::StreamPaymentServiceNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);

	DECLARE_HANDLER(StreamStartService, model::StreamStartServiceNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorService);
}}
