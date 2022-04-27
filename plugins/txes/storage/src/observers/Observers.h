/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/observers/ObserverTypes.h"
#include "src/config/StorageConfiguration.h"
#include "catapult/model/StorageNotifications.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "src/model/InternalStorageNotifications.h"
#include "src/cache/DownloadChannelCache.h"
#include "src/cache/BcDriveCache.h"
#include "src/cache/QueueCache.h"
#include "src/cache/ReplicatorCache.h"
#include "src/utils/StorageUtils.h"
#include <queue>

namespace catapult { namespace state { class StorageState; }}

namespace catapult { namespace observers {

	using DrivePriority = std::pair<Key, double>;
	using DriveQueue = std::priority_queue<DrivePriority, std::vector<DrivePriority>, utils::DriveQueueComparator>;

	/// Observes changes triggered by prepare drive notifications.
	DECLARE_OBSERVER(PrepareDrive, model::PrepareDriveNotification<1>)(const std::shared_ptr<cache::ReplicatorKeyCollector>& pKeyCollector, const std::shared_ptr<DriveQueue>& pDriveQueue);

	/// Observes changes triggered by download notifications.
	DECLARE_OBSERVER(DownloadChannel, model::DownloadNotification<1>)();

	/// Observes changes triggered by data modification notifications.
	DECLARE_OBSERVER(DataModification, model::DataModificationNotification<1>)(const std::shared_ptr<cache::ReplicatorKeyCollector>& pKeyCollector, const std::shared_ptr<DriveQueue>& pDriveQueue);

	/// Observes changes triggered by data modification approval notifications.
	DECLARE_OBSERVER(DataModificationApproval, model::DataModificationApprovalNotification<1>)();

	/// Observes changes triggered by data modification approval download work notifications.
	DECLARE_OBSERVER(DataModificationApprovalDownloadWork, model::DataModificationApprovalDownloadWorkNotification<1>)();

	/// Observes changes triggered by data modification approval upload work notifications.
	DECLARE_OBSERVER(DataModificationApprovalUploadWork, model::DataModificationApprovalUploadWorkNotification<1>)();

	/// Observes changes triggered by data modification approval refund notifications.
	DECLARE_OBSERVER(DataModificationApprovalRefund, model::DataModificationApprovalRefundNotification<1>)();

	/// Observes changes triggered by data modification cancel notifications.
	DECLARE_OBSERVER(DataModificationCancel, model::DataModificationCancelNotification<1>)();

	/// Observes changes triggered by replicator onboarding notifications.
	DECLARE_OBSERVER(ReplicatorOnboarding, model::ReplicatorOnboardingNotification<1>)(const std::shared_ptr<DriveQueue>& pDriveQueue);

	/// Observes changes triggered by drive closure notifications.
	DECLARE_OBSERVER(DriveClosure, model::DriveClosureNotification<1>)(const std::shared_ptr<DriveQueue>& pDriveQueue);

	/// Observes changes triggered by replicator offboarding notifications.
	DECLARE_OBSERVER(ReplicatorOffboarding, model::ReplicatorOffboardingNotification<1>)(const std::shared_ptr<DriveQueue>& pDriveQueue);

	/// Observes changes triggered by download payment notifications.
	DECLARE_OBSERVER(DownloadPayment, model::DownloadPaymentNotification<1>)();

	/// Observes changes triggered by data modification single approval notifications.
	DECLARE_OBSERVER(DataModificationSingleApproval, model::DataModificationSingleApprovalNotification<1>)();

	/// Observes changes triggered by download approval notifications.
	DECLARE_OBSERVER(DownloadApproval, model::DownloadApprovalNotification<1>)();

	/// Observes changes triggered by download approval payment notifications.
	DECLARE_OBSERVER(DownloadApprovalPayment, model::DownloadApprovalPaymentNotification<1>)();

	/// Observes changes triggered by download channel refund notifications.
	DECLARE_OBSERVER(DownloadChannelRefund, model::DownloadChannelRefundNotification<1>)();

	/// Observes changes triggered by stream start notifications.
	DECLARE_OBSERVER(StreamStart, model::StreamStartNotification<1>)();

	/// Observes changes triggered by stream finish notifications.
	DECLARE_OBSERVER(StreamFinish, model::StreamFinishNotification<1>)();

	/// Observes changes triggered by stream payment notifications.
	DECLARE_OBSERVER(StreamPayment, model::StreamPaymentNotification<1>)();

	/// Observes changes triggered by start drive verification notifications.
	DECLARE_OBSERVER(StartDriveVerification, model::BlockNotification<1>)(state::StorageState& state);

	/// Observes changes triggered by end drive verification notifications.
	DECLARE_OBSERVER(EndDriveVerification, model::EndDriveVerificationNotification<1>)(const std::shared_ptr<cache::ReplicatorKeyCollector>& pKeyCollector, const std::shared_ptr<DriveQueue>& pDriveQueue);

	/// Observes changes triggered by block
	DECLARE_OBSERVER(PeriodicStoragePayment, model::BlockNotification<1>)(const std::shared_ptr<DriveQueue>& pDriveQueue);

	/// Observes changes triggered by block
	DECLARE_OBSERVER(PeriodicDownloadChannelPayment, model::BlockNotification<1>)();
}}
