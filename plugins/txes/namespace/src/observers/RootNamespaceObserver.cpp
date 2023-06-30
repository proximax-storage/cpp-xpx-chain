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
#include "src/cache/NamespaceCache.h"
#include "catapult/constants.h"
#include "src/catapult/cache_core/AccountStateCache.h"
#include "src/catapult/cache_core/AccountStateCacheUtils.h"
#include <limits>

namespace catapult { namespace observers {

	namespace {
		bool IsRenewal(const cache::ReadOnlyAccountStateCache& cache, const state::RootNamespace& root, const model::RootNamespaceNotification<1>& notification, Height height) {

			return root.lifetime().isActiveOrGracePeriod(height) && cache::GetCurrentlyActiveAccountKey(cache, root.owner()) == notification.Signer;
		}

		Height LifetimeEnd(const Height& height, const BlockDuration& duration) {
			return Eternal_Artifact_Duration == duration
			   ? Height(std::numeric_limits<Height::ValueType>::max())
			   : height + Height(duration.unwrap());
		}
		void ObserveNotification(const model::RootNamespaceNotification<1>& notification, const ObserverContext& context) {
			auto& cache = context.Cache.sub<cache::NamespaceCache>();

			if (NotifyMode::Rollback == context.Mode) {
				cache.remove(notification.NamespaceId);
				return;
			}

			auto lifetimeEnd = LifetimeEnd(context.Height, notification.Duration);
			auto lifetime = state::NamespaceLifetime(context.Height, lifetimeEnd);
			bool isRenewal = false;
			if (cache.contains(notification.NamespaceId)) {
				// if a renewal, duration should add onto current expiry
				auto namespaceIter = cache.find(notification.NamespaceId);
				const auto& rootEntry = namespaceIter.get();
				const auto& accountStateCache = context.Cache.template sub<cache::AccountStateCache>();
				if (IsRenewal(accountStateCache.asReadOnly(), rootEntry.root(), notification, context.Height)) {
					lifetime = rootEntry.root().lifetime();
					lifetime.End = LifetimeEnd(lifetime.End, notification.Duration);
					isRenewal = true;
				}
			}

			auto root = state::RootNamespace(notification.NamespaceId, notification.Signer, lifetime);
			cache.insert(root, isRenewal);
		}
	}

	DEFINE_OBSERVER(RootNamespace, model::RootNamespaceNotification<1>, ObserveNotification);
}}
