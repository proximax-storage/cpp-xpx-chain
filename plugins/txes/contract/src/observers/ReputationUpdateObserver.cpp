/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/ReputationCache.h"

namespace catapult { namespace observers {

	using Notification = model::ReputationUpdateNotification;

	namespace {
		class AccountReputationFacade {
		public:
			explicit AccountReputationFacade(cache::ReputationCacheDelta& reputationCache)
					: m_reputationCache(reputationCache)
			{}

		public:
			void incrementPositiveInteractions(const Key& key) {
				getReputationEntry(key).incrementPositiveInteractions();
			}

			void decrementPositiveInteractions(const Key& key) {
				getReputationEntry(key).decrementPositiveInteractions();
			}

			void incrementNegativeInteractions(const Key& key) {
				getReputationEntry(key).incrementNegativeInteractions();
			}

			void decrementNegativeInteractions(const Key& key) {
				getReputationEntry(key).decrementNegativeInteractions();
			}

			void removeIfEmpty(const Key& key) {
				if (m_reputationCache.contains(key)) {
					auto& entry = m_reputationCache.find(key).get();
					if (!entry.positiveInteractions().unwrap() &&
						!entry.negativeInteractions().unwrap())
						m_reputationCache.remove(key);
				}
			}

		private:
			state::ReputationEntry& getReputationEntry(const Key& key) {
				if (!m_reputationCache.contains(key))
					m_reputationCache.insert(state::ReputationEntry(key));

				return m_reputationCache.find(key).get();
			}

		private:
			cache::ReputationCacheDelta& m_reputationCache;
		};
	}

	DEFINE_OBSERVER(ReputationUpdate, Notification, [](const auto& notification, const ObserverContext& context) {
		auto& reputationCache = context.Cache.sub<cache::ReputationCache>();

		AccountReputationFacade accountReputationFacade(reputationCache);
		auto isNotificationForward = NotifyMode::Commit == context.Mode;
		auto modificationsPtr = notification.ModificationsPtr();
		auto count = notification.ModificationCount();
		for (auto i = 0u; i < count; ++i) {
			const auto& modification = **modificationsPtr;
			const auto& key = modification.CosignatoryPublicKey;

			if (model::CosignatoryModificationType::Add == modification.ModificationType) {
				if (isNotificationForward)
					accountReputationFacade.incrementPositiveInteractions(key);
				else
					accountReputationFacade.decrementPositiveInteractions(key);
			} else {
				if (isNotificationForward)
					accountReputationFacade.incrementNegativeInteractions(key);
				else
					accountReputationFacade.decrementNegativeInteractions(key);
			}

			accountReputationFacade.removeIfEmpty(key);
			++modificationsPtr;
		}
	});
}}
