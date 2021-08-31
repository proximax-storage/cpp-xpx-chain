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
#include "src/cache/DownloadChannelCache.h"
#include "src/cache/BcDriveCache.h"
#include "src/cache/ReplicatorCache.h"
#include "src/cache/BlsKeysCache.h"

namespace catapult { namespace observers {

	/// Observes changes triggered by prepare drive notifications.
	DECLARE_OBSERVER(PrepareDrive, model::PrepareDriveNotification<1>)(const std::shared_ptr<cache::ReplicatorKeyCollector>& pKeyCollector);

	/// Observes changes triggered by download notifications.
	DECLARE_OBSERVER(DownloadChannel, model::DownloadNotification<1>)();

	/// Observes changes triggered by data modification notifications.
	DECLARE_OBSERVER(DataModification, model::DataModificationNotification<1>)();

	/// Observes changes triggered by data modification approval notifications.
	DECLARE_OBSERVER(DataModificationApproval, model::DataModificationApprovalNotification<1>)();

	/// Observes changes triggered by data modification cancel notifications.
	DECLARE_OBSERVER(DataModificationCancel, model::DataModificationCancelNotification<1>)();

	/// Observes changes triggered by replicator onboarding notifications.
	DECLARE_OBSERVER(ReplicatorOnboarding, model::ReplicatorOnboardingNotification<1>)();
	
	/// Observes changes triggered by replicator offboarding notifications.
	DECLARE_OBSERVER(ReplicatorOffboarding, model::ReplicatorOffboardingNotification<1>)();

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
}}
