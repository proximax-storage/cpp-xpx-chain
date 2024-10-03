/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER_WITH_LIQUIDITY_PROVIDER(StreamStart, model::StreamStartNotification<1>, ([&liquidityProvider, pStorageState](const model::StreamStartNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (StreamStart)");

	  	auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	auto driveIter = driveCache.find(notification.DriveKey);
	  	auto& driveEntry = driveIter.get();

		auto& activeDataModifications = driveEntry.activeDataModifications();
		activeDataModifications.emplace_back(state::ActiveDataModification(
			notification.StreamId,
			notification.Owner,
			notification.ExpectedUploadSize,
			notification.FolderName
		));

		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
		std::seed_seq seed(notification.StreamId.begin(), notification.StreamId.end());
		std::mt19937 rng(seed);

		const auto& offboardingReplicators = driveEntry.offboardingReplicators();
		const auto requiredReplicatorsCount = pluginConfig.MinReplicatorCount * 2 / 3 + 1;
		const auto maxOffboardingCount = driveEntry.replicators().size() < requiredReplicatorsCount ?
				0ul :
				driveEntry.replicators().size() - requiredReplicatorsCount;
		const auto offboardingCount = std::min(offboardingReplicators.size(), maxOffboardingCount);
		const auto actualOffboardingReplicators = std::set<Key>(
				offboardingReplicators.begin(),
				offboardingReplicators.begin() + offboardingCount);

		utils::RefundDepositsOnOffboarding(notification.DriveKey, actualOffboardingReplicators, context, liquidityProvider);
		utils::OffboardReplicatorsFromDrive(notification.DriveKey, actualOffboardingReplicators, context, rng);
		utils::PopulateDriveWithReplicators(notification.DriveKey, context, rng);

		auto& replicatorCache = context.Cache.template sub<cache::ReplicatorCache>();
		auto& downloadChannelCache = context.Cache.template sub<cache::DownloadChannelCache>();
		auto pDrive = utils::GetDrive(notification.DriveKey, pStorageState->replicatorKey(), context.Timestamp, driveCache, replicatorCache, downloadChannelCache);
		context.Notifications.push_back(std::make_unique<model::StreamStartServiceNotification<1>>(
			std::move(pDrive),
			notification.StreamId,
			notification.ExpectedUploadSize,
			notification.FolderName));
	}));
}}
