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

#include "catapult/validators/NotificationValidatorAdapter.h"
#include "tests/test/core/mocks/MockNotificationPublisher.h"
#include "tests/test/core/mocks/MockTransaction.h"
#include "tests/TestHarness.h"
#include "catapult/model/TransactionFeeCalculator.h"

namespace catapult { namespace validators {

#define TEST_CLASS NotificationValidatorAdapterTests

	namespace {
		ValidationResult ValidateEntity(const stateless::EntityValidator& validator, const model::VerifiableEntity& entity) {
			Hash256 hash;
			return validator.validate(model::WeakEntityInfo(entity, hash, Height{0}));
		}

		class MockNotificationValidator : public stateless::NotificationValidator {
		public:
			explicit MockNotificationValidator(const std::string& name, ValidationResult value)
					: m_name(name)
					, m_result(value)
					, m_numValidateCalls(0)
			{}

		public:
			const std::string& name() const override {
				return m_name;
			}

			ValidationResult validate(const model::Notification& notification) const override {
				++m_numValidateCalls;
				m_notificationTypes.push_back(notification.Type);

				if (model::Core_Signature_v1_Notification == notification.Type)
					m_signerKeys.push_back(static_cast<const model::SignatureNotification<1>&>(notification).Signer);

				return m_result;
			}

		public:
			size_t numValidateCalls() const {
				return m_numValidateCalls;
			}

			const auto& notificationTypes() const {
				return m_notificationTypes;
			}

			const auto& signerKeys() const {
				return m_signerKeys;
			}

		private:
			std::string m_name;
			ValidationResult m_result;
			mutable size_t m_numValidateCalls;
			mutable std::vector<model::NotificationType> m_notificationTypes;
			mutable std::vector<Key> m_signerKeys;
		};

		template<typename TRunTestFunc>
		void RunTest(ValidationResult result, TRunTestFunc runTest) {
			// Arrange:
			auto pValidator = std::make_unique<MockNotificationValidator>("alpha", result);
			const auto& validator = *pValidator;

			auto registry = mocks::CreateDefaultTransactionRegistry(mocks::PluginOptionFlags::Publish_Custom_Notifications);
			model::TransactionFeeCalculator transactionFeeCalculator;
			auto pPublisher = model::CreateNotificationPublisher(registry,
																 UnresolvedMosaicId(),
																 transactionFeeCalculator);
			NotificationValidatorAdapter adapter(std::move(pValidator), std::move(pPublisher));

			// Act + Assert:
			runTest(adapter, validator);
		}
	}

	TEST(TEST_CLASS, CanCreateAdapter) {
		// Arrange:
		RunTest(ValidationResult::Success, [](const auto& adapter, const auto&) {
			// Assert:
			EXPECT_EQ("alpha", adapter.name());
		});
	}

	TEST(TEST_CLASS, ExtractsAndForwardsNotificationsFromEntity) {
		// Arrange:
		RunTest(ValidationResult::Success, [](const auto& adapter, const auto& validator) {
			// Act:
			auto pTransaction = mocks::CreateMockTransaction(0);
			ValidateEntity(adapter, *pTransaction);

			// Assert: the mock transaction plugin sends additional public key notification and 6 custom notifications
			//         (notice that only 4/6 are raised on validator channel)
			ASSERT_EQ(6u + 4u, validator.notificationTypes().size());
			EXPECT_EQ(model::Core_Entity_v1_Notification, validator.notificationTypes()[0]);
			EXPECT_EQ(model::Core_Transaction_v1_Notification, validator.notificationTypes()[1]);
			EXPECT_EQ(model::Core_Transaction_Deadline_v1_Notification, validator.notificationTypes()[2]);
			EXPECT_EQ(model::Core_Transaction_Fee_v1_Notification, validator.notificationTypes()[3]);
			EXPECT_EQ(model::Core_Balance_Debit_v1_Notification, validator.notificationTypes()[4]);
			EXPECT_EQ(model::Core_Signature_v1_Notification, validator.notificationTypes()[5]);

			// - mock transaction notifications
			EXPECT_EQ(mocks::Mock_Validator_1_Notification, validator.notificationTypes()[6]);
			EXPECT_EQ(mocks::Mock_All_1_Notification, validator.notificationTypes()[7]);
			EXPECT_EQ(mocks::Mock_Validator_2_Notification, validator.notificationTypes()[8]);
			EXPECT_EQ(mocks::Mock_All_2_Notification, validator.notificationTypes()[9]);

			// - spot check the signer keys as a proxy for verifying data integrity
			ASSERT_EQ(1u, validator.signerKeys().size());
			EXPECT_EQ(pTransaction->Signer, validator.signerKeys()[0]);
		});
	}

	namespace {
		void AssertMockTransactionValidation(ValidationResult expectedResult, size_t expectedNumValidateCalls) {
			// Arrange:
			RunTest(expectedResult, [&](const auto& adapter, const auto& validator) {
				// Act:
				auto pTransaction = mocks::CreateMockTransaction(0);
				auto result = ValidateEntity(adapter, *pTransaction);

				// Assert:
				EXPECT_EQ(expectedResult, result);
				EXPECT_EQ(expectedNumValidateCalls, validator.numValidateCalls());
			});
		}
	}

	TEST(TEST_CLASS, DelegatesWhenTypeMatches_Success) {
		// Assert: all notifications should be processed
		AssertMockTransactionValidation(ValidationResult::Success, 10);
	}

	TEST(TEST_CLASS, DelegatesWhenTypeMatches_Neutral) {
		// Assert: all notifications should be processed
		AssertMockTransactionValidation(ValidationResult::Neutral, 10);
	}

	TEST(TEST_CLASS, DelegatesWhenTypeMatches_Failure) {
		// Assert: first failure short-circuits
		AssertMockTransactionValidation(ValidationResult::Failure, 1);
	}

	TEST(TEST_CLASS, CanSpecifyCustomPublisher) {
		// Arrange:
		auto pValidator = std::make_unique<MockNotificationValidator>("alpha", ValidationResult::Failure);
		const auto& validator = *pValidator;

		auto pPublisher = std::make_unique<mocks::MockNotificationPublisher>();
		const auto& publisher = *pPublisher;

		NotificationValidatorAdapter adapter(std::move(pValidator), std::move(pPublisher));

		auto pTransaction = mocks::CreateMockTransaction(0);

		// Act:
		auto result = ValidateEntity(adapter, *pTransaction);

		// Assert: the publisher shouldn't produce any notifications, so the validator should never get called
		EXPECT_EQ(ValidationResult::Success, result);
		EXPECT_EQ(1u, publisher.numPublishCalls());
		EXPECT_EQ(0u, validator.numValidateCalls());
	}
}}
