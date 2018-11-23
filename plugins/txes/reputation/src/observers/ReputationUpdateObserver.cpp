/**
*** Copyright (c) 2018-present,
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
#include "src/cache/ReputationCache.h"

namespace catapult { namespace observers {

	using Notification = model::ReputationUpdateNotification;

	namespace {
		class AccountReputationFacade {
		public:
			explicit AccountReputationFacade(cache::ReputationCacheDelta& reputationCache, const Key& accountKey)
					: m_reputationCache(reputationCache)
					, m_accountKey(accountKey)
					, m_reputationEntry(getReputationEntry(m_accountKey))
			{}

			~AccountReputationFacade() {
				removeIfEmpty();
			}

		public:
			void incrementPositiveInteractions(const Key& accountKey) {
				getReputationEntry(accountKey).incrementPositiveInteractions();
			}

			void decrementPositiveInteractions(const Key& accountKey) {
				getReputationEntry(accountKey).decrementPositiveInteractions();
			}

			void incrementNegativeInteractions(const Key& accountKey) {
				getReputationEntry(accountKey).incrementNegativeInteractions();
			}

			void decrementNegativeInteractions(const Key& accountKey) {
				getReputationEntry(accountKey).decrementNegativeInteractions();
			}

		private:
			state::ReputationEntry& getReputationEntry(const Key& key) {
				if (!m_reputationCache.contains(key))
					m_reputationCache.insert(state::ReputationEntry(key));

				return m_reputationCache.get(key);
			}

			void removeIfEmpty() {
				if (!m_reputationEntry.positiveInteractions().unwrap() &&
					!m_reputationEntry.negativeInteractions().unwrap())
					m_reputationCache.remove(m_accountKey);
			}

		private:
			cache::ReputationCacheDelta& m_reputationCache;
			const Key& m_accountKey;
			state::ReputationEntry& m_reputationEntry;
		};
	}

	DEFINE_OBSERVER(ReputationUpdate, Notification, [](const auto& notification, const ObserverContext& context) {
		auto& reputationCache = context.Cache.sub<cache::ReputationCache>();

		AccountReputationFacade accountReputationFacade(reputationCache, notification.Signer);
		const auto* pModifications = notification.ModificationsPtr;
		for (auto i = 0u; i < notification.ModificationsCount; ++i) {
			auto isNotificationAdd = model::CosignatoryModificationType::Add == pModifications[i].ModificationType;
			auto isNotificationForward = NotifyMode::Commit == context.Mode;
			const auto& key = pModifications[i].CosignatoryPublicKey;

			if (isNotificationAdd) {
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
		}
	});
}}
