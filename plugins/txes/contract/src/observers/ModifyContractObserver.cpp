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
#include "src/cache/ContractCache.h"
#include "src/config/ContractConfiguration.h"
#include "plugins/txes/multisig/src/cache/MultisigCache.h"
#include <math.h>

namespace catapult { namespace observers {

	using Notification = model::ModifyContractNotification;

	namespace {
		auto GetContractEntry(cache::ContractCacheDelta& contractCache, const Key& key) {
			if (!contractCache.contains(key))
				contractCache.insert(state::ContractEntry(key));

			return contractCache.find(key);
		}

		class ContractFacade {
		public:
			explicit ContractFacade(cache::ContractCacheDelta& contractCache, const Key& contractMultisigKey)
					: m_contractCache(contractCache)
					, m_contractEntry(GetContractEntry(m_contractCache, contractMultisigKey).get())
			{}

		public:
			Height start() const {
				return m_contractEntry.start();
			}

			void setStart(const Height& start) {
				m_contractEntry.setStart(start);
			}

			BlockDuration duration() const {
				return m_contractEntry.duration();
			}

			void setDuration(const BlockDuration& duration) {
				m_contractEntry.setDuration(duration);
			}

			void setHash(const Hash256& hash) {
				m_contractEntry.setHash(hash);
			}

			void addCustomer(const Key& customerKey) {
				m_contractEntry.customers().insert(customerKey);
			}

			void removeCustomer(const Key& customerKey) {
				m_contractEntry.customers().erase(customerKey);
			}

			void addExecutor(const Key& executorKey) {
				m_contractEntry.executors().insert(executorKey);
			}

			void removeExecutor(const Key& executorKey) {
				m_contractEntry.executors().erase(executorKey);
			}

			void addVerifier(const Key& verifierKey) {
				m_contractEntry.verifiers().insert(verifierKey);
			}

			void removeVerifier(const Key& verifierKey) {
				m_contractEntry.verifiers().erase(verifierKey);
			}

			uint8_t verifierCount() {
				return m_contractEntry.verifiers().size();
			}

		private:
			cache::ContractCacheDelta& m_contractCache;
			state::ContractEntry& m_contractEntry;
		};
	}

	DECLARE_OBSERVER(ModifyContract, Notification)(config::ContractConfiguration config) {
		return MAKE_OBSERVER(ModifyContract, Notification, [config](const auto& notification, const ObserverContext& context) {
			auto& contractCache = context.Cache.sub<cache::ContractCache>();
			ContractFacade contractFacade(contractCache, notification.Multisig);

			if (contractFacade.start() == Height{0u}) {
				contractFacade.setStart(context.Height);
				contractFacade.setDuration(BlockDuration{static_cast<uint64_t>(notification.DurationDelta)});
			} else {
				contractFacade.setDuration(BlockDuration{
						contractFacade.duration().unwrap() + static_cast<uint64_t>(notification.DurationDelta)});
			}
			contractFacade.setHash(notification.Hash);

#define MODIFY_CONTRACTORS(CONTRACTOR_TYPE) \
		for (auto i = 0u; i < notification.CONTRACTOR_TYPE##ModificationCount; ++i) { \
			auto isNotificationAdd = model::CosignatoryModificationType::Add == notification.CONTRACTOR_TYPE##ModificationsPtr[i].ModificationType; \
			auto isNotificationForward = NotifyMode::Commit == context.Mode; \
			if (isNotificationAdd == isNotificationForward) \
				contractFacade.add##CONTRACTOR_TYPE(notification.CONTRACTOR_TYPE##ModificationsPtr[i].CosignatoryPublicKey); \
			else \
				contractFacade.remove##CONTRACTOR_TYPE(notification.CONTRACTOR_TYPE##ModificationsPtr[i].CosignatoryPublicKey); \
		}

			MODIFY_CONTRACTORS(Customer)
			MODIFY_CONTRACTORS(Executor)
			MODIFY_CONTRACTORS(Verifier)

			auto& multisigCache = context.Cache.sub<cache::MultisigCache>();
			auto& multisigEntry = multisigCache.find(notification.Multisig).get();
			float verifierCount = contractFacade.verifierCount();
			multisigEntry.setMinApproval(ceil(verifierCount * config.MinPercentageOfApproval / 100));
			multisigEntry.setMinRemoval(ceil(verifierCount * config.MinPercentageOfRemoval / 100));
		});
	}
}}
