/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/ContractCache.h"
#include "plugins/txes/multisig/src/cache/MultisigCache.h"
#include <math.h>

namespace catapult { namespace observers {

	using Notification = model::ModifyContractNotification<1>;

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
			void changeDuration(const int64_t& delta, Height height) {
				if (m_contractEntry.start().unwrap() == 0)
					m_contractEntry.setStart(height);

				m_contractEntry.setDuration(BlockDuration{m_contractEntry.duration().unwrap() + delta});

				if (m_contractEntry.start() == height && m_contractEntry.duration().unwrap() == 0)
					m_contractEntry.setStart(Height(0));
			}

			void pushHash(const Hash256& hash, const Height& height) {
				m_contractEntry.pushHash(hash, height);
			}

			void popHash(const Hash256& hash, const Height& height) {
				auto lastSnapshot = m_contractEntry.hashes().back();
				if (hash != lastSnapshot.Hash || height != lastSnapshot.HashHeight)
					CATAPULT_THROW_RUNTIME_ERROR("during rollback we can remove only last hash snapshot");

				m_contractEntry.popHash();
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

	DECLARE_OBSERVER(ModifyContract, Notification)() {
		return MAKE_OBSERVER(ModifyContract, Notification, [](const auto& notification, const ObserverContext& context) {
			auto& contractCache = context.Cache.sub<cache::ContractCache>();
			ContractFacade contractFacade(contractCache, notification.Multisig);

			if (NotifyMode::Commit == context.Mode) {
				contractFacade.changeDuration(notification.DurationDelta, context.Height);
				contractFacade.pushHash(notification.Hash, context.Height);
			} else {
				contractFacade.changeDuration(-notification.DurationDelta, context.Height);
				contractFacade.popHash(notification.Hash, context.Height);
			}

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
			auto multisigEntryIter = multisigCache.find(notification.Multisig);
		  	auto& multisigEntry = multisigEntryIter.get();
			float verifierCount = contractFacade.verifierCount();
			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::ContractConfiguration>();
			multisigEntry.setMinApproval(ceil(verifierCount * pluginConfig.MinPercentageOfApproval / 100));
			multisigEntry.setMinRemoval(ceil(verifierCount * pluginConfig.MinPercentageOfRemoval / 100));
		});
	}
}}
