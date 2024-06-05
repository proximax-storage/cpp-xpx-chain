/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/utils/AVLTree.h"

namespace catapult { namespace observers {

	using Notification = model::ReplicatorTreeRebuildNotification<1>;

	DECLARE_OBSERVER(ReplicatorTreeRebuild, Notification)() {
		return MAKE_OBSERVER(ReplicatorTreeRebuild, Notification, ([](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ReplicatorTreeRebuild)");

		  	auto& queueCache = context.Cache.template sub<cache::QueueCache>();
			auto& replicatorCache = context.Cache.template sub<cache::ReplicatorCache>();
		  	auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();

		  	const auto& storageMosaicId = context.Config.Immutable.StorageMosaicId;

			// Resetting the root node of the queue.
		  	if (queueCache.contains(state::ReplicatorsSetTree)) {
				auto& entry = queueCache.find(state::ReplicatorsSetTree).get();
				entry.setFirst(Key());
				entry.setLast(Key());
				entry.setSize(0);
			} else {
				state::QueueEntry entry(state::ReplicatorsSetTree);
				queueCache.insert(entry);
			}

			// Resetting nodes stored in the replicator entries.
			auto pReplicatorKey = notification.ReplicatorKeysPtr;
			for (auto i = 0u; i < notification.ReplicatorCount; ++i, ++pReplicatorKey) {
				auto replicatorIter = replicatorCache.find(*pReplicatorKey);
				auto& replicatorEntry = replicatorIter.get();
				replicatorEntry.replicatorsSetNode() = state::AVLTreeNode();
			}

		  	// Reinserting replicators into the tree.
		  	utils::AVLTreeAdapter<std::pair<Amount, Key>> replicatorTreeAdapter(
					queueCache,
				  	state::ReplicatorsSetTree,
					[&storageMosaicId, &accountStateCache](const Key& key) {
						return std::make_pair(accountStateCache.find(key).get().Balances.get(storageMosaicId), key);
					},
				  	[&replicatorCache](const Key& key) -> state::AVLTreeNode {
						return replicatorCache.find(key).get().replicatorsSetNode();
				  	},
				  	[&replicatorCache](const Key& key, const state::AVLTreeNode& node) {
						replicatorCache.find(key).get().replicatorsSetNode() = node;
				  	});

			pReplicatorKey = notification.ReplicatorKeysPtr;
		  	for (auto i = 0u; i < notification.ReplicatorCount; ++i, ++pReplicatorKey) {
				replicatorTreeAdapter.insert(*pReplicatorKey);
		  	}
        }))
	};
}}
