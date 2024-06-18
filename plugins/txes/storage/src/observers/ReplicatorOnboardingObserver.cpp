/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/utils/StorageUtils.h"

namespace catapult { namespace observers {

	namespace {
		template<VersionType version>
		void ReplicatorOnboardingObserver(const model::ReplicatorOnboardingNotification<version>& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ReplicatorOnboarding)");

			auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
			state::ReplicatorEntry replicatorEntry(notification.PublicKey);
			replicatorEntry.setVersion(version);
			replicatorCache.insert(replicatorEntry);

			std::seed_seq seed(notification.Seed.begin(), notification.Seed.end());
			std::mt19937 rng(seed);
			utils::AssignReplicatorsToQueuedDrives({notification.PublicKey}, context, rng);
		}
	}

	DECLARE_OBSERVER(ReplicatorOnboardingV1, model::ReplicatorOnboardingNotification<1>)() {
		return MAKE_OBSERVER(ReplicatorOnboardingV1, model::ReplicatorOnboardingNotification<1>, ([](const model::ReplicatorOnboardingNotification<1>& notification, ObserverContext& context) {
			ReplicatorOnboardingObserver(notification, context);
		}))
	}

	DECLARE_OBSERVER(ReplicatorOnboardingV2, model::ReplicatorOnboardingNotification<2>)() {
		return MAKE_OBSERVER(ReplicatorOnboardingV2, model::ReplicatorOnboardingNotification<2>, ([](const model::ReplicatorOnboardingNotification<2>& notification, ObserverContext& context) {
			ReplicatorOnboardingObserver(notification, context);
		}))
	}
}}
