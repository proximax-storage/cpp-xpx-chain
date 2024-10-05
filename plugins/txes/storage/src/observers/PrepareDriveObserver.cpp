/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"
#include "src/utils/Queue.h"
#include "src/utils/AVLTree.h"

namespace catapult { namespace observers {

	template<VersionType version>
	using Notification = model::PrepareDriveNotification<version>;

	namespace {
		template<VersionType version>
		void PrepareDriveObserver(const Notification<version>& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (PrepareDrive)");

			auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
			state::BcDriveEntry driveEntry(notification.DriveKey);
			driveEntry.setOwner(notification.Owner);
			driveEntry.setSize(notification.DriveSize);
			driveEntry.setReplicatorCount(notification.ReplicatorCount);
			driveEntry.setLastPayment(context.Timestamp);

			driveCache.insert(driveEntry);

			std::seed_seq seed(notification.DriveKey.begin(), notification.DriveKey.end());
			std::mt19937 rng(seed);

			PopulateDriveWithReplicators(notification.DriveKey, context, rng);

			// Insert the Drive into the payment Queue
			auto& queueCache = context.Cache.template sub<cache::QueueCache>();
			utils::QueueAdapter<cache::BcDriveCache> queueAdapter(queueCache, state::DrivePaymentQueueKey, driveCache);
			queueAdapter.pushBack(driveEntry.entryKey());

			// Insert the Drive into the Verification Queue
			utils::AVLTreeAdapter<Key> treeAdapter(
					queueCache,
					state::DriveVerificationsTree,
					[](const Key& key) { return key; },
					[&driveCache](const Key& key) -> state::AVLTreeNode& {
						return driveCache.find(key).get().verificationNode();
					},
					[&driveCache](const Key& key, const state::AVLTreeNode& node) {
						return driveCache.find(key).get().verificationNode() = node;
					});
			treeAdapter.insert(driveEntry.key());
		}
	}

	DECLARE_OBSERVER(PrepareDriveV1, Notification<1>)() {
		return MAKE_OBSERVER(PrepareDriveV1, Notification<1>, ([](const Notification<1>& notification, ObserverContext& context) {
		  	PrepareDriveObserver(notification, context);
		}))
	}

	DECLARE_OBSERVER(PrepareDriveV2, Notification<2>)() {
		return MAKE_OBSERVER(PrepareDriveV2, Notification<2>, ([](const Notification<2>& notification, ObserverContext& context) {
		  	PrepareDriveObserver(notification, context);
		}))
	}
}}
