/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "Observers.h"
#include "src/cache/MosaicCache.h"
#include "src/cache/NamespaceCache.h"

namespace catapult { namespace observers {

	using Notification = model::RootNamespaceNotification;

	namespace {
		bool IsOwnerExtendingRootLifetime(const Key& signer, const state::RootNamespace& root, Height height) {
			return root.owner() == signer && root.lifetime().isActiveOrGracePeriod(height);
		}
	}

	DECLARE_OBSERVER(RegisterNamespaceMosaicPruning, Notification)(BlockDuration maxRollbackBlocks) {
		return MAKE_OBSERVER(RegisterNamespaceMosaicPruning, Notification, [maxRollbackBlocks](
				const auto& notification,
				const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				return;

			if (context.Height.unwrap() < maxRollbackBlocks.unwrap())
				return;

			// note that the pruning observer should run before the transaction itself is observed
			const auto& namespaceCache = context.Cache.sub<cache::NamespaceCache>();
			auto namespaceIter = namespaceCache.find(notification.NamespaceId);
			if (!namespaceIter.tryGet())
				return;

			const auto& root = namespaceIter.get().root();
			if (IsOwnerExtendingRootLifetime(notification.Signer, root, Height(context.Height.unwrap() - maxRollbackBlocks.unwrap())))
				return;

			auto& mosaicCache = context.Cache.sub<cache::MosaicCache>();
			mosaicCache.remove(root.id());
			for (const auto& pair : root.children())
				mosaicCache.remove(pair.first);
		});
	}
}}
