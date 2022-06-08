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

#include "plugins/txes/aggregate/src/config/AggregateConfiguration.h"
#include "Validators.h"
#include "src/cache/MultisigCache.h"
#include "catapult/model/TransactionPlugin.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	namespace {
		template<typename TNotification>
		class AggregateCosignaturesChecker {
		public:
			explicit AggregateCosignaturesChecker(
					const TNotification& notification,
					const model::TransactionRegistry& transactionRegistry,
					const cache::MultisigCache::CacheReadOnlyType& multisigCache,
					const config::BlockchainConfiguration& config)
					: m_notification(notification)
					, m_transactionRegistry(transactionRegistry)
					, m_multisigCache(multisigCache)
					, m_config(config) {
				
				const auto& pluginConfig = config.Network.template GetPluginConfiguration<config::AggregateConfiguration>();
				
				if(pluginConfig.StrictSigner)
					m_cosigners.emplace(&m_notification.Signer, false);
				
				for (auto i = 0u; i < m_notification.CosignaturesCount; ++i)
					m_cosigners.emplace(&m_notification.CosignaturesPtr[i].Signer(), false);
			}

		public:
			bool hasIneligibleCosigners() {
				// find all eligible cosigners
				const auto* pTransaction = m_notification.TransactionsPtr;
				for (auto i = 0u; i < m_notification.TransactionsCount; ++i) {
					findEligibleCosigners(pTransaction->Signer);

					const auto& transactionPlugin = m_transactionRegistry.findPlugin(pTransaction->Type)->embeddedPlugin();
					for (const auto& requiredCosigner : transactionPlugin.additionalRequiredCosigners(*pTransaction, m_config))
						findEligibleCosigners(requiredCosigner);

					pTransaction = model::AdvanceNext(pTransaction);
				}

				// check if all cosigners are in fact eligible
				return std::any_of(m_cosigners.cbegin(), m_cosigners.cend(), [](const auto& pair) {
					return !pair.second;
				});
			}

		private:
			void findEligibleCosigners(const Key& publicKey) {
				// if the account is unknown or not multisig, only the public key itself is eligible
				if (!m_multisigCache.contains(publicKey)) {
					markEligible(publicKey);
					return;
				}

				// if the account is a cosignatory only, treat it as non-multisig
				auto multisigIter = m_multisigCache.find(publicKey);
				const auto& multisigEntry = multisigIter.get();
				if (multisigEntry.cosignatories().empty()) {
					markEligible(publicKey);
					return;
				}

				// if the account is multisig, only its cosignatories are eligible
				for (const auto& cosignatoryPublicKey : multisigEntry.cosignatories())
					findEligibleCosigners(cosignatoryPublicKey);
			}

			void markEligible(const Key& key) {
				auto iter = m_cosigners.find(&key);
				if (m_cosigners.cend() != iter)
					iter->second = true;
			}

		private:
			const TNotification& m_notification;
			const model::TransactionRegistry& m_transactionRegistry;
			const cache::MultisigCache::CacheReadOnlyType& m_multisigCache;
			utils::ArrayPointerFlagMap<Key> m_cosigners;
			const config::BlockchainConfiguration& m_config;
		};
		template<typename TNotification>
		ValidationResult Validate(const TNotification& notification, const ValidatorContext& context, const model::TransactionRegistry& transactionRegistry)
		{
			AggregateCosignaturesChecker checker(notification, transactionRegistry, context.Cache.sub<cache::MultisigCache>(), context.Config);
			return checker.hasIneligibleCosigners() ? Failure_Aggregate_Ineligible_Cosigners : ValidationResult::Success;
		}
	}


	DECLARE_STATEFUL_VALIDATOR(MultisigAggregateEligibleCosignersV1, model::AggregateCosignaturesNotification<1>)(const model::TransactionRegistry& transactionRegistry) {
		return MAKE_STATEFUL_VALIDATOR_WITH_TYPE(MultisigAggregateEligibleCosignersV1, model::AggregateCosignaturesNotification<1>, [&transactionRegistry](
				const model::AggregateCosignaturesNotification<1>& notification,
				const ValidatorContext& context) {
			return Validate<model::AggregateCosignaturesNotification<1>>(notification, context, transactionRegistry);
		});
	}
	DECLARE_STATEFUL_VALIDATOR(MultisigAggregateEligibleCosignersV2, model::AggregateCosignaturesNotification<3>)(const model::TransactionRegistry& transactionRegistry) {
		return MAKE_STATEFUL_VALIDATOR_WITH_TYPE(MultisigAggregateEligibleCosignersV2, model::AggregateCosignaturesNotification<3>, [&transactionRegistry](
				const model::AggregateCosignaturesNotification<3>& notification,
				const ValidatorContext& context) {
		  return Validate<model::AggregateCosignaturesNotification<3>>(notification, context, transactionRegistry);
		});
	}
}}
