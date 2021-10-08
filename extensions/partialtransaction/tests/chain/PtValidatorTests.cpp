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

#include "partialtransaction/src/chain/PtValidator.h"
#include "plugins/txes/aggregate/src/model/AggregateNotifications.h"
#include "plugins/txes/aggregate/src/validators/Results.h"
#include "catapult/model/WeakCosignedTransactionInfo.h"
#include "catapult/plugins/PluginManager.h"
#include "partialtransaction/tests/test/AggregateTransactionTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/mocks/MockCapturingNotificationValidator.h"
#include "tests/test/other/mocks/MockNotification.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/TestHarness.h"

using namespace catapult::validators;

namespace catapult { namespace chain {

#define TEST_CLASS PtValidatorTests

	namespace {
		template<typename TNotification>
		using StatefulValidatorPointer = std::unique_ptr<mocks::MockCapturingStatefulNotificationValidator<TNotification>>;
		template<typename TNotification>
		using StatelessValidatorPointer = std::unique_ptr<mocks::MockCapturingStatelessNotificationValidator<TNotification>>;

		constexpr auto Default_Block_Time = Timestamp(987);

		enum class ValidatorType { Stateful, Stateless };

		class ValidationResultOptions {
		public:
			ValidationResultOptions(ValidationResult result)
					: m_result(result)
					, m_errorType(ValidatorType::Stateful)
					, m_triggerOnSpecificNotificationType(false)
			{}

			ValidationResultOptions(ValidatorType errorType, model::NotificationType triggerType)
					: m_result(ValidationResult::Failure)
					, m_errorType(errorType)
					, m_triggerOnSpecificNotificationType(true)
					, m_triggerType(triggerType)
			{}

		public:
			ValidationResult result() const {
				return m_result;
			}

			bool isStatefulResult() const {
				return ValidatorType::Stateful == m_errorType;
			}

			template<typename TValidator>
			void setResult(TValidator& validator) const {
				if (m_triggerOnSpecificNotificationType)
					validator.setResult(m_result, m_triggerType);
				else
					validator.setResult(m_result);
			}

		private:
			ValidationResult m_result;
			ValidatorType m_errorType;
			bool m_triggerOnSpecificNotificationType;
			model::NotificationType m_triggerType;
		};

		class TestContext {
		public:
			explicit TestContext(const ValidationResultOptions& options)
					: m_cache(test::CreateEmptyCatapultCache())
					, m_pluginManager(config::CreateMockConfigurationHolder(), plugins::StorageConfiguration()) {
				// Arrange: register mock support (for validatePartial)
				auto pluginOptionFlags = mocks::PluginOptionFlags::Publish_Custom_Notifications;
				m_pluginManager.addTransactionSupport(mocks::CreateMockTransactionPlugin(pluginOptionFlags));

#define ADD_VALIDATORS(VALIDATOR_TYPE) \
				std::vector<const mocks::BasicMockNotificationValidator*> subBasicValidators; \
				std::vector<const test::ParamsCapture<mocks::VALIDATOR_TYPE##NotificationValidatorParams>*> subParamCapturers; \
				builder.add(create##VALIDATOR_TYPE##Validator<model::AggregateCosignaturesNotification<1>>(options, subBasicValidators, subParamCapturers)); \
				builder.add(create##VALIDATOR_TYPE##Validator<model::AggregateEmbeddedTransactionNotification<1>>(options, subBasicValidators, subParamCapturers)); \
				builder.add(create##VALIDATOR_TYPE##Validator<model::EntityNotification<1>>(options, subBasicValidators, subParamCapturers)); \
				builder.add(create##VALIDATOR_TYPE##Validator<model::TransactionNotification<1>>(options, subBasicValidators, subParamCapturers)); \
				builder.add(create##VALIDATOR_TYPE##Validator<model::TransactionDeadlineNotification<1>>(options, subBasicValidators, subParamCapturers)); \
				builder.add(create##VALIDATOR_TYPE##Validator<model::TransactionFeeNotification<1>>(options, subBasicValidators, subParamCapturers)); \
				builder.add(create##VALIDATOR_TYPE##Validator<model::BalanceDebitNotification<1>>(options, subBasicValidators, subParamCapturers)); \
				builder.add(create##VALIDATOR_TYPE##Validator<model::SignatureNotification<1>>(options, subBasicValidators, subParamCapturers));             \
				builder.add(create##VALIDATOR_TYPE##Validator<model::SignatureNotification<2>>(options, subBasicValidators, subParamCapturers)); \
				builder.add(create##VALIDATOR_TYPE##Validator<mocks::MockNotification<mocks::Mock_Validator_1_Notification>>(options, subBasicValidators, subParamCapturers)); \
				basicValidators.push_back(subBasicValidators); \
				paramCapturers.push_back(subParamCapturers);

				// - register a validator that will return the desired result
				if (options.isStatefulResult()) {
					m_pluginManager.addStatefulValidatorHook([&options, &basicValidators = m_basicValidators, &paramCapturers = m_statefulParamCapturers](auto& builder) {
						ADD_VALIDATORS(Stateful);
					});
				} else {
					m_pluginManager.addStatelessValidatorHook([&options, &basicValidators = m_basicValidators, &paramCapturers = m_statelessParamCapturers](auto& builder) {
						ADD_VALIDATORS(Stateless);
					});
				}

				m_pValidator = CreatePtValidator(m_cache, []() { return Default_Block_Time; }, m_pluginManager);
			}

		public:
			const auto& validator() {
				return *m_pValidator;
			}

			auto notificationTypesAt(size_t index) {
				return notificationTypes(m_basicValidators[index]);
			}

			auto statefulValidatorCapturedParamsAt(size_t index) {
				return capturedParams(m_statefulParamCapturers[index]);
			}

			auto statelessValidatorCapturedParamsAt(size_t index) {
				return capturedParams(m_statelessParamCapturers[index]);
			}

		private:
			template<typename TNotification>
			static std::unique_ptr<const stateful::NotificationValidatorT<TNotification>> createStatefulValidator(
					const ValidationResultOptions& options,
					std::vector<const mocks::BasicMockNotificationValidator*>& basicValidators,
					std::vector<const test::ParamsCapture<mocks::StatefulNotificationValidatorParams>*>& paramCapturers) {
				auto pValidator = std::make_unique<typename StatefulValidatorPointer<TNotification>::element_type>();
				options.setResult(*pValidator);
				basicValidators.push_back(static_cast<const mocks::BasicMockNotificationValidator*>(pValidator.get()));
				paramCapturers.push_back(static_cast<const test::ParamsCapture<mocks::StatefulNotificationValidatorParams>*>(pValidator.get()));

				return pValidator;
			}

			template<typename TNotification>
			static std::unique_ptr<const stateless::NotificationValidatorT<TNotification>> createStatelessValidator(
					const ValidationResultOptions& options,
					std::vector<const mocks::BasicMockNotificationValidator*>& basicValidators,
					std::vector<const test::ParamsCapture<mocks::StatelessNotificationValidatorParams>*>& paramCapturers) {
				auto pValidator = std::make_unique<typename StatelessValidatorPointer<TNotification>::element_type>();
				options.setResult(*pValidator);
				basicValidators.push_back(static_cast<const mocks::BasicMockNotificationValidator*>(pValidator.get()));
				paramCapturers.push_back(static_cast<const test::ParamsCapture<mocks::StatelessNotificationValidatorParams>*>(pValidator.get()));

				return pValidator;
			}

			std::vector<model::NotificationType> notificationTypes(const std::vector<const mocks::BasicMockNotificationValidator*>& basicValidatorPtrs) {
				std::vector<model::NotificationType> result;
				for (const auto* pBasicValidator : basicValidatorPtrs) {
					const auto& notificationTypes = pBasicValidator->notificationTypes();
					result.insert(result.end(), notificationTypes.begin(), notificationTypes.end());
				}

				return result;
			}

			template<typename TParams>
			std::vector<TParams> capturedParams(const std::vector<const test::ParamsCapture<TParams>*>& paramCapturers) {
				std::vector<TParams> result;
				for (const auto* paramCapturer : paramCapturers) {
					const auto& params = paramCapturer->params();
					result.insert(result.end(), params.begin(), params.end());
				}

				return result;
			}

		private:
			cache::CatapultCache m_cache;
			plugins::PluginManager m_pluginManager;
			std::unique_ptr<PtValidator> m_pValidator;
			std::vector<std::vector<const mocks::BasicMockNotificationValidator*>> m_basicValidators;
			std::vector<std::vector<const test::ParamsCapture<mocks::StatefulNotificationValidatorParams>*>> m_statefulParamCapturers;
			std::vector<std::vector<const test::ParamsCapture<mocks::StatelessNotificationValidatorParams>*>> m_statelessParamCapturers;
		};
	}

	// region validatePartial

	namespace {
		struct ValidatePartialResult {
			bool IsValid;
			bool IsShortCircuited;
		};

		template<typename TCapturedParams>
		void ValidateTransactionNotifications(
				const TCapturedParams& capturedParams,
				const Hash256& expectedHash,
				Timestamp expectedDeadline,
				const std::string& description) {
			// Assert:
			auto i = 0u;
			auto numMatches = 0u;
			for (const auto& params : capturedParams) {
				++i;
				if (!params.TransactionNotificationInfo.IsSet)
					continue;

				auto message = description + " - notification at " + std::to_string(i);
				EXPECT_EQ(expectedHash, params.TransactionNotificationInfo.TransactionHash) << message;
				EXPECT_EQ(expectedDeadline, params.TransactionNotificationInfo.Deadline) << message;
				++numMatches;
			}

			EXPECT_LE(0u, numMatches) << description;
		}

		template<typename TNotificationTypesConsumer>
		void RunValidatePartialTest(
				const ValidationResultOptions& validationResultOptions,
				bool isValid,
				TNotificationTypesConsumer notificationTypesConsumer) {
			// Arrange:
			TestContext context(validationResultOptions);
			const auto& validator = context.validator();

			// Act: validatePartial does not filter transactions even though, in practice, it will only be called with aggregates
			auto pTransaction = mocks::CreateMockTransaction(0);
			auto transactionHash = test::GenerateRandomByteArray<Hash256>();
			auto result = validator.validatePartial(model::WeakEntityInfoT<model::Transaction>(*pTransaction, transactionHash, Height{0}));

			// Assert: when valid, raw result should always be success (even when validationResult is suppressed failure)
			EXPECT_EQ(isValid, result.Normalized);
			EXPECT_EQ(isValid ? ValidationResult::Success : validationResultOptions.result(), result.Raw);

			// - short circuiting on failure
			auto notificationTypes = context.notificationTypesAt(0); // partial
			notificationTypesConsumer(notificationTypes);

			// - correct timestamp was passed to validator
			auto params = context.statefulValidatorCapturedParamsAt(0); // partial
			ASSERT_LE(1u, params.size());
			EXPECT_EQ(Default_Block_Time, params[0].BlockTime);

			// - correct transaction information is passed down (if validation wasn't short circuited)
			if (notificationTypes.size() > 1)
				ValidateTransactionNotifications(params, transactionHash, pTransaction->Deadline, "basic");
		}

		void RunValidatePartialTest(const ValidationResultOptions& validationResultOptions, const ValidatePartialResult& expectedResult) {
			// Arrange:
			RunValidatePartialTest(validationResultOptions, expectedResult.IsValid, [&expectedResult](const auto& notificationTypes) {
				// Assert:
				if (!expectedResult.IsShortCircuited) {
					ASSERT_EQ(6u, notificationTypes.size());
					EXPECT_EQ(model::Core_Entity_v1_Notification, notificationTypes[0]);
					EXPECT_EQ(model::Core_Transaction_v1_Notification, notificationTypes[1]);
					EXPECT_EQ(model::Core_Transaction_Deadline_v1_Notification, notificationTypes[2]);
					EXPECT_EQ(model::Core_Transaction_Fee_v1_Notification, notificationTypes[3]);
					EXPECT_EQ(model::Core_Balance_Debit_v1_Notification, notificationTypes[4]);
					EXPECT_EQ(model::Core_Signature_v1_Notification, notificationTypes[5]);
				} else {
					ASSERT_EQ(1u, notificationTypes.size());
					EXPECT_EQ(model::Core_Entity_v1_Notification, notificationTypes[0]);
				}
			});
		}
	}

	TEST(TEST_CLASS, ValidatePartialMapsSuccessToTrue) {
		// Assert:
		RunValidatePartialTest(ValidationResult::Success, { true, false });
	}

	TEST(TEST_CLASS, ValidatePartialMapsNeutralToFalse) {
		// Assert:
		RunValidatePartialTest(ValidationResult::Neutral, { false, false });
	}

	TEST(TEST_CLASS, ValidatePartialMapsGenericFailureToFalse) {
		// Assert:
		RunValidatePartialTest(ValidationResult::Failure, { false, true });
	}

	TEST(TEST_CLASS, ValidatePartialMapsFailureAggregateIneligibleCosignersToFalse) {
		// Assert:
		RunValidatePartialTest(Failure_Aggregate_Ineligible_Cosigners, { false, true });
	}

	TEST(TEST_CLASS, ValidatePartialMapsFailureAggregateMissingCosignersToTrue) {
		// Assert:
		RunValidatePartialTest(Failure_Aggregate_Missing_Cosigners, { true, false });
	}

	TEST(TEST_CLASS, ValidatePartialMapsBasicStatefulFailureToFalse) {
		// Arrange:
		auto options = ValidationResultOptions{ ValidatorType::Stateful, model::Core_Transaction_v1_Notification };
		RunValidatePartialTest(options, false, [](const auto& notificationTypes) {
			// Assert:
			ASSERT_EQ(2u, notificationTypes.size());
			EXPECT_EQ(model::Core_Entity_v1_Notification, notificationTypes[0]);
			EXPECT_EQ(model::Core_Transaction_v1_Notification, notificationTypes[1]);
		});
	}

	TEST(TEST_CLASS, ValidatePartialMapsCustomStatefulFailureToTrue) {
		// Assert: custom stateful validators are bypassed
		RunValidatePartialTest({ ValidatorType::Stateful, mocks::Mock_Validator_1_Notification }, { true, false });
	}

	namespace {
		template<typename TNotificationTypesConsumer>
		void RunInvalidStatelessValidatePartialTest(
				model::NotificationType notificationType,
				TNotificationTypesConsumer notificationTypesConsumer) {
			// Arrange:
			ValidationResultOptions validationResultOptions{ ValidatorType::Stateless, notificationType };
			TestContext context(validationResultOptions);
			const auto& validator = context.validator();

			// Act: validatePartial does not filter transactions even though, in practice, it will only be called with aggregates
			auto pTransaction = mocks::CreateMockTransaction(0);
			auto transactionHash = test::GenerateRandomByteArray<Hash256>();
			auto result = validator.validatePartial(model::WeakEntityInfoT<model::Transaction>(*pTransaction, transactionHash, Height{0}));

			// Assert:
			EXPECT_FALSE(result.Normalized);
			EXPECT_EQ(validationResultOptions.result(), result.Raw);

			// - merge notification types from both validators
			auto allNotificationTypes = context.notificationTypesAt(0); // partial (basic)
			auto customNotificationTypes = context.notificationTypesAt(1); // partial (custom)
			allNotificationTypes.insert(allNotificationTypes.end(), customNotificationTypes.cbegin(), customNotificationTypes.cend());
			notificationTypesConsumer(allNotificationTypes);

			// - correct transaction information is passed down
			ValidateTransactionNotifications(context.statelessValidatorCapturedParamsAt(0), transactionHash, pTransaction->Deadline, "basic");
		}
	}

	TEST(TEST_CLASS, ValidatePartialMapsBasicStatelessFailureToFalse) {
		// Arrange:
		RunInvalidStatelessValidatePartialTest(model::Core_Transaction_v1_Notification, [](const auto& notificationTypes) {
			// Assert:
			ASSERT_EQ(2u, notificationTypes.size());
			EXPECT_EQ(model::Core_Entity_v1_Notification, notificationTypes[0]);
			EXPECT_EQ(model::Core_Transaction_v1_Notification, notificationTypes[1]);
		});
	}

	TEST(TEST_CLASS, ValidatePartialMapsCustomStatelessFailureToFalse) {
		// Arrange:
		RunInvalidStatelessValidatePartialTest(mocks::Mock_Validator_1_Notification, [](const auto& notificationTypes) {
			// Assert:
			ASSERT_EQ(7u, notificationTypes.size());
			EXPECT_EQ(model::Core_Entity_v1_Notification, notificationTypes[0]);
			EXPECT_EQ(model::Core_Transaction_v1_Notification, notificationTypes[1]);
			EXPECT_EQ(model::Core_Transaction_Deadline_v1_Notification, notificationTypes[2]);
			EXPECT_EQ(model::Core_Transaction_Fee_v1_Notification, notificationTypes[3]);
			EXPECT_EQ(model::Core_Balance_Debit_v1_Notification, notificationTypes[4]);
			EXPECT_EQ(model::Core_Signature_v1_Notification, notificationTypes[5]);
			EXPECT_EQ(mocks::Mock_Validator_1_Notification, notificationTypes[6]);
		});
	}

	// endregion

	// region validateCosigners

	namespace {
		struct ValidateCosignersResult {
			CosignersValidationResult Result;
			bool IsShortCircuited;
		};

		auto CreateAggregateTransaction(uint8_t numTransactions) {
			return test::CreateAggregateTransaction(numTransactions).pTransaction;
		}

		void RunValidateCosignersTest(ValidationResult validationResult, const ValidateCosignersResult& expectedResult) {
			// Arrange:
			TestContext context(validationResult);
			const auto& validator = context.validator();

			// Act:
			auto pTransaction = CreateAggregateTransaction(2);
			auto cosignatures = test::GenerateRandomDataVector<model::Cosignature<CoSignatureVersionAlias::Raw>>(3);
			auto result = validator.validateCosigners({ pTransaction.get(), &cosignatures });

			// Assert:
			EXPECT_EQ(expectedResult.Result, result.Normalized);
			EXPECT_EQ(validationResult, result.Raw);

			// - short circuiting on failure
			auto notificationTypes = context.notificationTypesAt(1); // cosigners
			if (!expectedResult.IsShortCircuited) {
				ASSERT_EQ(3u, notificationTypes.size());
				EXPECT_EQ(model::Aggregate_Cosignatures_v1_Notification, notificationTypes[0]);
				EXPECT_EQ(model::Aggregate_EmbeddedTransaction_v1_Notification, notificationTypes[1]);
				EXPECT_EQ(model::Aggregate_EmbeddedTransaction_v1_Notification, notificationTypes[2]);
			} else {
				ASSERT_EQ(1u, notificationTypes.size());
				EXPECT_EQ(model::Aggregate_Cosignatures_v1_Notification, notificationTypes[0]);
			}

			// - correct timestamp was passed to validator
			auto params = context.statefulValidatorCapturedParamsAt(1); // cosigners
			ASSERT_LE(1u, params.size());
			EXPECT_EQ(Default_Block_Time, params[0].BlockTime);
		}
	}

	TEST(TEST_CLASS, ValidateCosignersMapsSuccessToSuccess) {
		// Assert:
		RunValidateCosignersTest(ValidationResult::Success, { CosignersValidationResult::Success, false });
	}

	TEST(TEST_CLASS, ValidateCosignersMapsNeutralToFailure) {
		// Assert:
		RunValidateCosignersTest(ValidationResult::Neutral, { CosignersValidationResult::Failure, false });
	}

	TEST(TEST_CLASS, ValidateCosignersMapsGenericFailureToFailure) {
		// Assert:
		RunValidateCosignersTest(ValidationResult::Failure, { CosignersValidationResult::Failure, true });
	}

	TEST(TEST_CLASS, ValidateCosignersMapsFailureAggregateIneligibleCosignersToIneligible) {
		// Assert:
		RunValidateCosignersTest(Failure_Aggregate_Ineligible_Cosigners, { CosignersValidationResult::Ineligible, true });
	}

	TEST(TEST_CLASS, ValidateCosignersMapsFailureAggregateMissingCosignersToMissing) {
		// Assert:
		RunValidateCosignersTest(Failure_Aggregate_Missing_Cosigners, { CosignersValidationResult::Missing, true });
	}

	// endregion
}}
