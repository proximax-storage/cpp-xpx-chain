/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/QueueCache.h"
#include "src/cache/PriorityQueueCache.h"
#include "src/utils/AVLTree.h"

namespace catapult { namespace validators {

	template<VersionType version>
	using Notification = model::PrepareDriveNotification<version>;

	namespace {
		template<VersionType version>
		ValidationResult PrepareDriveValidator(const Notification<version>& notification, const ValidatorContext& context) {
			const auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();

			// Check if drive size >= minDriveSize
			if (utils::FileSize::FromMegabytes(notification.DriveSize) < pluginConfig.MinDriveSize)
				return Failure_Storage_Drive_Size_Insufficient;

			// Check if drive size <= maxDriveSize
			if (utils::FileSize::FromMegabytes(notification.DriveSize) > pluginConfig.MaxDriveSize)
				return Failure_Storage_Drive_Size_Excessive;

			// Check if number of replicators >= minReplicatorCount
			if (notification.ReplicatorCount < pluginConfig.MinReplicatorCount)
				return Failure_Storage_Replicator_Count_Insufficient;

			if (notification.ReplicatorCount > pluginConfig.MaxReplicatorCount)
				return Failure_Storage_Replicator_Count_Exceeded;

			// Check if the drive already exists
			if (driveCache.contains(notification.DriveKey))
				return Failure_Storage_Drive_Already_Exists;

			return ValidationResult::Success;
		}
	}

	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(PrepareDriveV1, Notification<1>, ([](const Notification<1>& notification, const ValidatorContext& context) {
	  	const ValidationResult result = PrepareDriveValidator(notification, context);
		return result;
	}))

	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(PrepareDriveV2, Notification<2>, ([](const Notification<2>& notification, const ValidatorContext& context) {
		const ValidationResult result = PrepareDriveValidator(notification, context);
		if (result != ValidationResult::Success)
			return result;

	  	auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
	  	auto& priorityQueueCache = context.Cache.sub<cache::PriorityQueueCache>();
	  	auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		const auto& storageMosaicId = context.Config.Immutable.StorageMosaicId;

	  	// Filter out replicators that are ready to be assigned to the drive,
	  	// i.e. which have at least (notification.DriveSize) of storage units
	  	// and at least (2 * notification.DriveSize) of streaming units:

	  	auto keyExtractor = [=, &accountStateCache](const Key& key) {
			return std::make_pair(accountStateCache.find(key).get().Balances.get(storageMosaicId), key);
	  	};

		utils::ReadOnlyAVLTreeAdapter<std::pair<Amount, Key>> treeAdapter(
				context.Cache.template sub<cache::QueueCache>(),
			  	state::ReplicatorsSetTree,
			  	keyExtractor,
			  	[&replicatorCache](const Key& key) -> state::AVLTreeNode {
					return replicatorCache.find(key).get().replicatorsSetNode();
			  	});

	  	// Calculating the number of replicators ready to be assigned to the drive.
	  	auto notSuitableReplicators = treeAdapter.numberOfLess({Amount(notification.DriveSize), Key()});
	  	auto suitableReplicators = treeAdapter.size() - notSuitableReplicators;

		// Check if Drive Owner is present among suitable replicators.
		// If he is, account for it (drive owners cannot be assigned to their own drives).
	  	bool ownerIsReplicator = replicatorCache.contains(notification.Owner);
		bool ownerHasEnoughStorageMosaics = accountStateCache.find(notification.Owner).get().Balances.get(storageMosaicId) >= Amount(notification.DriveSize);

		if (ownerIsReplicator && ownerHasEnoughStorageMosaics)
			suitableReplicators -= 1;

		if (suitableReplicators < notification.ReplicatorCount)
			return Failure_Storage_Not_Enough_Suitable_Replicators;

		return ValidationResult::Success;
	}))
}}
