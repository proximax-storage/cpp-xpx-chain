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

#include <plugins/txes/aggregate/src/model/AggregateTransaction.h>
#include "PtValidator.h"
#include "AggregateCosignersNotificationPublisher.h"
#include "JointValidator.h"
#include "plugins/txes/aggregate/src/validators/Results.h"
#include "catapult/model/WeakCosignedTransactionInfo.h"
#include "catapult/plugins/PluginManager.h"
#include "catapult/validators/NotificationValidatorAdapter.h"
#include "catapult/validators/ValidatingNotificationSubscriber.h"

using namespace catapult::validators;

namespace catapult { namespace chain {

	namespace {
		constexpr bool IsMissingCosignersResult(ValidationResult result) {
			return Failure_Aggregate_Missing_Cosigners == result;
		}

		constexpr CosignersValidationResult MapToCosignersValidationResult(ValidationResult result) {
			if (IsValidationResultSuccess(result))
				return CosignersValidationResult::Success;

			// map failures (not using switch statement to workaround gcc warning)
			return Failure_Aggregate_Ineligible_Cosigners == result
					? CosignersValidationResult::Ineligible
					: Failure_Aggregate_Missing_Cosigners == result
							? CosignersValidationResult::Missing
							: CosignersValidationResult::Failure;
		}

		class DefaultPtValidator : public PtValidator {
		public:
			DefaultPtValidator(
					const cache::CatapultCache& cache,
					const TimeSupplier& timeSupplier,
					const plugins::PluginManager& pluginManager)
					: m_transactionValidator(
							CreateJointValidator(cache, timeSupplier, pluginManager, IsMissingCosignersResult),
							pluginManager.createNotificationPublisher(model::PublicationMode::Basic))
					, m_statelessTransactionValidator(
							pluginManager.createStatelessValidator(),
							pluginManager.createNotificationPublisher(model::PublicationMode::Custom))
					, m_pCosignersValidator(CreateJointValidator(cache, timeSupplier, pluginManager, [](auto) { return false; }))
					, m_pluginManager(pluginManager)
			{}

		public:
			Result<bool> validatePartial(const model::WeakEntityInfoT<model::Transaction>& transactionInfo) const override {
				// notice that partial validation has two differences relative to "normal" validation
				// 1. missing cosigners failures are ignored
				// 2. custom stateful validators are ignored
				// 3. only top level aggregate signer check
				if(!isABTSignerAllowed(transactionInfo)) {
					CATAPULT_LOG(debug) << "partial transaction failed validation of aggregate signer(s)";
					return {ValidationResult::Failure, false};
				}

				auto weakEntityInfo = transactionInfo.cast<model::VerifiableEntity>();
				auto result = m_transactionValidator.validate(weakEntityInfo);
				if (IsValidationResultSuccess(result)) {
					// - check custom stateless validators
					result = m_statelessTransactionValidator.validate(weakEntityInfo);
					if (IsValidationResultSuccess(result))
						return { result, true };
				}

				CATAPULT_LOG_LEVEL(validators::MapToLogLevel(result)) << "partial transaction failed validation with " << result;
				return { result, false };
			}

			Result<CosignersValidationResult> validateCosigners(const model::WeakCosignedTransactionInfo& transactionInfo) const override {
				validators::ValidatingNotificationSubscriber sub(*m_pCosignersValidator);
				m_aggregatePublisher.publish(transactionInfo, sub);
				return { sub.result(), MapToCosignersValidationResult(sub.result()) };
			}
		private:
			const model::EmbeddedTransaction* moveForward(const model::EmbeddedTransaction* pTransaction) const {
				const auto* pTransactionData = reinterpret_cast<const uint8_t*>(pTransaction);
				return reinterpret_cast<const model::EmbeddedTransaction*>(pTransactionData + pTransaction->Size);
			}

			bool isABTSignerAllowed( const model::WeakEntityInfoT<model::Transaction>& transactionInfo) const {

				if(m_pluginManager.config().AllowNonParticipantSigner)
					return true;

				try {
					auto weakEntityInfo = transactionInfo.cast<model::VerifiableEntity>();
					auto pAggregate = weakEntityInfo.cast<const model::AggregateTransaction>();
					const auto *pTransaction = pAggregate.entity().TransactionsPtr();
					const auto *pCosignature = pAggregate.entity().CosignaturesPtr();

					if(pAggregate.entity().Type != model::Entity_Type_Aggregate_Bonded){
						return false;
					}

					auto cosignersCount = pAggregate.entity().CosignaturesCount();
					auto transactionsCount = static_cast<uint32_t>(std::distance(
							pAggregate.entity().Transactions().cbegin(),
							pAggregate.entity().Transactions().cend()));

					utils::ArrayPointerFlagMap<Key> cosigners;
					cosigners.emplace(&pAggregate.entity().Signer, false);
					for (auto i = 0u; i < cosignersCount; ++i) {
						cosigners.emplace(&pCosignature->Signer, false);
						++pCosignature;
					}

					for (auto i = 0u; i < transactionsCount; ++i) {
						for (auto it = cosigners.begin(); it != cosigners.end(); it++) {
							auto iter = cosigners.find(&pTransaction->Signer);
							if (cosigners.cend() != iter)
								iter->second = true;
						}

						pTransaction = moveForward(pTransaction);
					}

					return std::any_of(cosigners.cbegin(), cosigners.cend(), [](const auto &pair) {
						return pair.second;
					});

				}catch(...) {
					// unexpected error, probably that entity() is NULL or not set
					CATAPULT_LOG(debug) << "Unexpected error while checking for signer in partial transaction ";
					return false;
				}
			}
		private:
			NotificationValidatorAdapter m_transactionValidator;
			NotificationValidatorAdapter m_statelessTransactionValidator;
			AggregateCosignersNotificationPublisher m_aggregatePublisher;
			std::unique_ptr<const stateless::NotificationValidator> m_pCosignersValidator;
			const plugins::PluginManager& m_pluginManager;
		};
	}

	std::unique_ptr<PtValidator> CreatePtValidator(
			const cache::CatapultCache& cache,
			const TimeSupplier& timeSupplier,
			const plugins::PluginManager& pluginManager) {
		return std::make_unique<DefaultPtValidator>(cache, timeSupplier, pluginManager);
	}
}}
