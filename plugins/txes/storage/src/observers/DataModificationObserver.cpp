/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::DataModificationNotification<1>;

	DEFINE_OBSERVER_WITH_LIQUIDITY_PROVIDER(DataModification, Notification, ([&liquidityProvider, pStorageState](const Notification& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DataModification)");

		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		auto driveIter = driveCache.find(notification.DriveKey);
		auto& driveEntry = driveIter.get();

		bool localReplicatorAssigned = (driveEntry.replicators().find(pStorageState->replicatorKey()) != driveEntry.replicators().end());

		auto& activeDataModifications = driveEntry.activeDataModifications();
		activeDataModifications.emplace_back(state::ActiveDataModification(
			notification.DataModificationId,
			notification.Owner,
			notification.DownloadDataCdi,
			notification.UploadSizeMegabytes
		));

		std::seed_seq seed(notification.DataModificationId.begin(), notification.DataModificationId.end());
		std::mt19937 rng(seed);

		const auto offboardingReplicators = driveEntry.offboardingReplicators();
		const auto requiredReplicatorsCount = pluginConfig.MinReplicatorCount * 2 / 3 + 1;
		const auto maxOffboardingCount = driveEntry.replicators().size() < requiredReplicatorsCount ? 0ul : driveEntry.replicators().size() - requiredReplicatorsCount;
		const auto offboardingCount = std::min(offboardingReplicators.size(), maxOffboardingCount);
		const auto actualOffboardingReplicators = std::set<Key>(offboardingReplicators.begin(), offboardingReplicators.begin() + offboardingCount);

		utils::RefundDepositsOnOffboarding(notification.DriveKey, actualOffboardingReplicators, context, liquidityProvider);
		utils::OffboardReplicatorsFromDrive(notification.DriveKey, actualOffboardingReplicators, context, rng);
		utils::PopulateDriveWithReplicators(notification.DriveKey, context, rng);

		auto& replicatorCache = context.Cache.template sub<cache::ReplicatorCache>();
		auto& downloadChannelCache = context.Cache.template sub<cache::DownloadChannelCache>();
		auto pDrive = utils::GetDrive(notification.DriveKey, pStorageState->replicatorKey(), context.Timestamp, driveCache, replicatorCache, downloadChannelCache);
		context.Notifications.push_back(std::make_unique<model::DataModificationServiceNotification<1>>(std::move(pDrive), notification.DataModificationId, notification.DownloadDataCdi, notification.UploadSizeMegabytes));
	}));
}}
