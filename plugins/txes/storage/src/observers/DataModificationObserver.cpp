/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/utils/StorageUtils.h"

namespace catapult { namespace observers {

	using Notification = model::DataModificationNotification<1>;

	DECLARE_OBSERVER(DataModification, Notification)(const LiquidityProviderExchangeObserver& liquidityProvider) {
		return MAKE_OBSERVER(DataModification, Notification, ([&liquidityProvider](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DataModification)");

		  	const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
			auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
			auto driveIter = driveCache.find(notification.DriveKey);
			auto& driveEntry = driveIter.get();

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
			const auto maxOffboardingCount = driveEntry.replicators().size() < requiredReplicatorsCount ?
											 0ul :
											 driveEntry.replicators().size() - requiredReplicatorsCount;
			const auto offboardingCount = std::min(offboardingReplicators.size(), maxOffboardingCount);
			const auto actualOffboardingReplicators = std::set<Key>(
					offboardingReplicators.begin(),
					offboardingReplicators.begin() + offboardingCount);

			utils::RefundDepositsToReplicators(notification.DriveKey, actualOffboardingReplicators, context, liquidityProvider);
			utils::OffboardReplicatorsFromDrive(notification.DriveKey, actualOffboardingReplicators, context, rng);
		  	utils::PopulateDriveWithReplicators(notification.DriveKey, context, rng);
		}))
	}
}}
