/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	namespace {
		template<VersionType version>
		void ReplicatorOnboardingObserver(
				const model::ReplicatorOnboardingNotification<version>& notification,
				ObserverContext& context,
				const std::shared_ptr<state::StorageState>& pStorageState) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ReplicatorOnboarding)");

			auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
			state::ReplicatorEntry replicatorEntry(notification.PublicKey);
			replicatorEntry.setVersion(version);
			replicatorCache.insert(replicatorEntry);

			std::seed_seq seed(notification.Seed.begin(), notification.Seed.end());
			std::mt19937 rng(seed);
			utils::AssignReplicatorsToQueuedDrives(pStorageState->replicatorKey(), { notification.PublicKey }, context, rng);

			CATAPULT_LOG(warning) << "======================================================> replicator on-boarding observer: on-boarded replicator " << notification.PublicKey;
			context.Notifications.push_back(std::make_unique<model::ReplicatorOnboardingServiceNotification<1>>(notification.PublicKey));
		}
	}

	DECLARE_OBSERVER(ReplicatorOnboardingV1, model::ReplicatorOnboardingNotification<1>)(const std::shared_ptr<state::StorageState>& pStorageState) {
		return MAKE_OBSERVER(ReplicatorOnboardingV1, model::ReplicatorOnboardingNotification<1>, ([pStorageState](const model::ReplicatorOnboardingNotification<1>& notification, ObserverContext& context) {
			ReplicatorOnboardingObserver(notification, context, pStorageState);
		}))
	}

	DECLARE_OBSERVER(ReplicatorOnboardingV2, model::ReplicatorOnboardingNotification<2>)(const std::shared_ptr<state::StorageState>& pStorageState) {
		return MAKE_OBSERVER(ReplicatorOnboardingV2, model::ReplicatorOnboardingNotification<2>, ([pStorageState](const model::ReplicatorOnboardingNotification<2>& notification, ObserverContext& context) {
			ReplicatorOnboardingObserver(notification, context, pStorageState);
		}))
	}
}}
