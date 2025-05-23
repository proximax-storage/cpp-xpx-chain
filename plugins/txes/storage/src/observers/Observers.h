/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/cache/BcDriveCache.h"
#include "src/cache/DownloadChannelCache.h"
#include "src/cache/QueueCache.h"
#include "src/cache/ReplicatorCache.h"
#include "src/config/StorageConfiguration.h"
#include "src/model/InternalStorageNotifications.h"
#include "src/model/StorageReceiptType.h"
#include "src/utils/StorageUtils.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/model/ServiceStorageNotifications.h"
#include "catapult/model/StorageNotifications.h"
#include "catapult/observers/DbrbProcessUpdateListener.h"
#include "catapult/observers/LiquidityProviderExchangeObserver.h"
#include "catapult/observers/ObserverTypes.h"
#include "catapult/observers/StorageUpdatesListener.h"
#include "catapult/utils/StorageUtils.h"
#include <queue>

namespace catapult { namespace state { class StorageState; }}

namespace catapult { namespace observers {

	class StorageDbrbProcessUpdateListener : public DbrbProcessUpdateListener {
	public:
		explicit StorageDbrbProcessUpdateListener(
				const std::unique_ptr<observers::LiquidityProviderExchangeObserver>& pLiquidityProvider,
				const std::shared_ptr<state::StorageState>& pStorageState)
			: m_pLiquidityProvider(pLiquidityProvider)
			, m_pStorageState(pStorageState)
		{}

		~StorageDbrbProcessUpdateListener() override = default;

	public:
		void OnDbrbProcessRemoved(ObserverContext& context, const dbrb::ProcessId& processId) const override;

	private:
		const std::unique_ptr<observers::LiquidityProviderExchangeObserver>& m_pLiquidityProvider;
		std::shared_ptr<state::StorageState> m_pStorageState;
	};

#define DEFINE_OBSERVER_WITH_LIQUIDITY_PROVIDER(NAME, NOTIFICATION_TYPE, HANDLER)																\
	DECLARE_OBSERVER(NAME, NOTIFICATION_TYPE)																									\
	(const std::unique_ptr<LiquidityProviderExchangeObserver>& liquidityProvider, const std::shared_ptr<state::StorageState>& pStorageState) {	\
		return MAKE_OBSERVER(NAME, NOTIFICATION_TYPE, HANDLER);																					\
	}

	/// Observes changes triggered by prepare drive notifications.
	DECLARE_OBSERVER(PrepareDrive, model::PrepareDriveNotification<1>)(const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by download notifications.
	DECLARE_OBSERVER(DownloadChannel, model::DownloadNotification<1>)(const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by data modification notifications.
	DECLARE_OBSERVER(DataModification, model::DataModificationNotification<1>)(
		const std::unique_ptr<LiquidityProviderExchangeObserver>& liquidityProvider,
		const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by data modification approval notifications.
	DECLARE_OBSERVER(DataModificationApproval, model::DataModificationApprovalNotification<1>)(const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by data modification approval download work notifications.
	DECLARE_OBSERVER(DataModificationApprovalDownloadWork, model::DataModificationApprovalDownloadWorkNotification<1>)(
		const std::unique_ptr<LiquidityProviderExchangeObserver>& liquidityProvider,
		const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by data modification approval upload work notifications.
	DECLARE_OBSERVER(DataModificationApprovalUploadWork, model::DataModificationApprovalUploadWorkNotification<1>)(
		const std::unique_ptr<LiquidityProviderExchangeObserver>& liquidityProvider,
		const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by data modification approval refund notifications.
	DECLARE_OBSERVER(DataModificationApprovalRefund, model::DataModificationApprovalRefundNotification<1>)(
		const std::unique_ptr<LiquidityProviderExchangeObserver>& liquidityProvider,
		const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by data modification cancel notifications.
	DECLARE_OBSERVER(DataModificationCancel, model::DataModificationCancelNotification<1>)(
		const std::unique_ptr<LiquidityProviderExchangeObserver>& liquidityProvider,
		const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by replicator onboarding notifications V1.
	DECLARE_OBSERVER(ReplicatorOnboardingV1, model::ReplicatorOnboardingNotification<1>)(const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by replicator onboarding notifications V2.
	DECLARE_OBSERVER(ReplicatorOnboardingV2, model::ReplicatorOnboardingNotification<2>)(const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by drive closure notifications.
	DECLARE_OBSERVER(DriveClosure, model::DriveClosureNotification<1>)(
		const std::unique_ptr<LiquidityProviderExchangeObserver>& liquidityProvider,
		const std::vector<std::unique_ptr<StorageUpdatesListener>>& updatesListeners,
		const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by replicator offboarding notifications.
	DECLARE_OBSERVER(ReplicatorOffboarding, model::ReplicatorOffboardingNotification<1>)();

	/// Observes changes triggered by download payment notifications.
	DECLARE_OBSERVER(DownloadPayment, model::DownloadPaymentNotification<1>)(const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by data modification single approval notifications.
	DECLARE_OBSERVER(DataModificationSingleApproval, model::DataModificationSingleApprovalNotification<1>)(const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by download approval notifications.
	DECLARE_OBSERVER(DownloadApproval, model::DownloadApprovalNotification<1>)(const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by download approval payment notifications.
	DECLARE_OBSERVER(DownloadApprovalPayment, model::DownloadApprovalPaymentNotification<1>)(
		const std::unique_ptr<LiquidityProviderExchangeObserver>& liquidityProvider,
		const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by download channel refund notifications.
	DECLARE_OBSERVER(DownloadChannelRefund, model::DownloadChannelRefundNotification<1>)(
		const std::unique_ptr<LiquidityProviderExchangeObserver>& liquidityProvider,
		const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes change triggered by finish download
	DECLARE_OBSERVER(FinishDownload, model::FinishDownloadNotification<1>)(const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by stream start notifications.
	DECLARE_OBSERVER(StreamStart, model::StreamStartNotification<1>)(
		const std::unique_ptr<LiquidityProviderExchangeObserver>& liquidityProvider,
		const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by stream finish notifications.
	DECLARE_OBSERVER(StreamFinish, model::StreamFinishNotification<1>)(const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by stream payment notifications.
	DECLARE_OBSERVER(StreamPayment, model::StreamPaymentNotification<1>)(const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by start drive verification notifications.
	DECLARE_OBSERVER(StartDriveVerification, model::BlockNotification<1>)(const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by end drive verification notifications.
	DECLARE_OBSERVER(EndDriveVerification, model::EndDriveVerificationNotification<1>)(
		const std::unique_ptr<LiquidityProviderExchangeObserver>& liquidityProvider,
		const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by block
	DECLARE_OBSERVER(PeriodicStoragePayment, model::BlockNotification<1>)(
		const std::unique_ptr<LiquidityProviderExchangeObserver>& liquidityProvider,
		const std::vector<std::unique_ptr<StorageUpdatesListener>>& updatesListeners,
		const std::shared_ptr<state::StorageState>& pStorageState);

	/// Observes changes triggered by block
	DECLARE_OBSERVER(PeriodicDownloadChannelPayment, model::BlockNotification<1>)(const std::shared_ptr<state::StorageState>& pStorageState);

	DECLARE_OBSERVER(OwnerManagementProhibition, model::OwnerManagementProhibitionNotification<1>)();

	/// Observes changes triggered by replicator / boot key notifications
	DECLARE_OBSERVER(ReplicatorNodeBootKey, model::ReplicatorNodeBootKeyNotification<1>)();

	/// Observes changes triggered by replicators cleanup notifications
	DECLARE_OBSERVER(ReplicatorsCleanupV1, model::ReplicatorsCleanupNotification<1>)(const std::unique_ptr<LiquidityProviderExchangeObserver>& pLiquidityProvider);
	DECLARE_OBSERVER(ReplicatorsCleanupV2, model::ReplicatorsCleanupNotification<2>)();

}}
