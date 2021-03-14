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

#include "catapult/validators/DemuxValidatorBuilder.h"
#include "catapult/cache/CatapultCache.h"
#include "tests/catapult/validators/test/MockTaggedBreadcrumbValidator.h"
#include "tests/test/other/mocks/MockNotification.h"
#include "tests/test/other/mocks/MockNotificationValidator.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS DemuxValidatorBuilderTests

	namespace {
		struct TestContext {
		public:
			std::vector<uint16_t> Breadcrumbs;
			std::unique_ptr<const stateful::AggregateNotificationValidator> pDemuxValidator;
			size_t NumIsSuppressedFailureCalls = 0;

		public:
			ValidationResult validate(uint8_t notificationId) {
				auto cache = test::CreateEmptyCatapultCache();
				return test::ValidateNotification<model::Notification>(*pDemuxValidator, test::TaggedNotification(notificationId), cache);
			}
		};

		std::unique_ptr<TestContext> CreateTestContext(size_t numValidators, bool varyValidators = false) {
			auto pContext = std::make_unique<TestContext>();
			stateful::DemuxValidatorBuilder builder;
			for (auto i = 0u; i < numValidators; ++i) {
				auto id = static_cast<uint8_t>(i + 1);
				if (!varyValidators || 1 == id % 2)
					builder.add(mocks::CreateTaggedBreadcrumbValidator(id, pContext->Breadcrumbs));
				else
					builder.add(mocks::CreateTaggedBreadcrumbValidator2(id, pContext->Breadcrumbs));
			}

			auto pDemuxValidator = builder.build([&count = pContext->NumIsSuppressedFailureCalls](auto) {
				++count;
				return false;
			});
			pContext->pDemuxValidator = std::move(pDemuxValidator);
			return pContext;
		}
	}

	// region basic delegation

	TEST(TEST_CLASS, CanCreateEmptyDemuxValidator) {
		// Act:
		auto pContext = CreateTestContext(0);
		auto result = pContext->validate(7);

		// Assert:
		EXPECT_EQ(ValidationResult::Success, result);

		std::vector<std::string> expectedNames;
		EXPECT_EQ(0u, pContext->Breadcrumbs.size());
		EXPECT_EQ("{}", pContext->pDemuxValidator->name());
		EXPECT_EQ(expectedNames, pContext->pDemuxValidator->names());
		EXPECT_EQ(0u, pContext->NumIsSuppressedFailureCalls);
	}

	TEST(TEST_CLASS, CanCreateDemuxValidatorWithMultipleValidators) {
		// Act:
		auto pContext = CreateTestContext(10);
		auto result = pContext->validate(12);

		// Assert:
		EXPECT_EQ(ValidationResult::Success, result);

		std::vector<std::string> expectedNames{ "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" };
		EXPECT_EQ(10u, pContext->Breadcrumbs.size());
		EXPECT_EQ("{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }", pContext->pDemuxValidator->name());
		EXPECT_EQ(expectedNames, pContext->pDemuxValidator->names());
		EXPECT_EQ(10u, pContext->NumIsSuppressedFailureCalls);
	}

	TEST(TEST_CLASS, AddAllowsChaining) {
		// Arrange:
		auto pContext = std::make_unique<TestContext>();
		stateful::DemuxValidatorBuilder builder;

		// Act:
		builder
			.add(mocks::CreateTaggedBreadcrumbValidator(2, pContext->Breadcrumbs))
			.add(mocks::CreateTaggedBreadcrumbValidator2(3, pContext->Breadcrumbs))
			.add(mocks::CreateTaggedBreadcrumbValidator(4, pContext->Breadcrumbs));
		pContext->pDemuxValidator = builder.build([](auto) { return false; });

		// Act:
		auto result = pContext->validate(7);

		// Assert:
		EXPECT_EQ(ValidationResult::Success, result);

		std::vector<uint16_t> expectedBreadcrumbs{ 0x0702, 0x0704 };
		EXPECT_EQ(2u, pContext->Breadcrumbs.size());
		EXPECT_EQ(expectedBreadcrumbs, pContext->Breadcrumbs);
	}

	// endregion

	// region validate

	TEST(TEST_CLASS, AggregateValidatorForwardsToAllValidatorsOnSuccess) {
		// Act:
		auto pContext = CreateTestContext(5, true);
		auto result = pContext->validate(4);

		// Assert:
		EXPECT_EQ(ValidationResult::Success, result);

		std::vector<uint16_t> expectedBreadcrumbs{ 0x0401, 0x0403, 0x0405 };
		EXPECT_EQ(3u, pContext->Breadcrumbs.size());
		EXPECT_EQ(expectedBreadcrumbs, pContext->Breadcrumbs);
	}

	TEST(TEST_CLASS, AggregateValidatorCanPerformMultipleValidations) {
		// Act:
		auto pContext = CreateTestContext(3, true);
		pContext->validate(2);
		pContext->validate(7);
		pContext->validate(5);
		pContext->validate(1);

		// Assert:
		std::vector<uint16_t> expectedBreadcrumbs{
			0x0201, 0x0203,
			0x0701, 0x0703,
			0x0501, 0x0503,
			0x0101, 0x0103,
		};
		EXPECT_EQ(8u, pContext->Breadcrumbs.size());
		EXPECT_EQ(expectedBreadcrumbs, pContext->Breadcrumbs);
	}

	// endregion

	// region forwarding

#define NOTIFICATION(i) mocks::MockNotification<static_cast<model::NotificationType>(i)>
#define VALIDATOR(i) mocks::MockNotificationValidatorT<NOTIFICATION(i)>

#define ADD_VALIDATOR(i) { \
        std::unique_ptr<const validators::stateful::NotificationValidatorT<NOTIFICATION(i)>> pMockValidator = std::make_unique<VALIDATOR(i)>(std::to_string(i)); \
        validators.push_back(pMockValidator.get()); \
        builder.add(std::move(pMockValidator)); \
	}

#define ASSERT_VALIDATOR(i, GET_VALUES, EXPECTED_VALUE) { \
		const auto& values = static_cast<const VALIDATOR(i)*>(validators[i])->GET_VALUES(); \
		const auto message = "validator at " + std::to_string(i); \
		if (2 == i) { \
			ASSERT_EQ(1u, values.size()) << message; \
			EXPECT_EQ(EXPECTED_VALUE, values[0]) << message; \
		} else { \
			EXPECT_EQ(0u, values.size()) << message; \
		} \
	}

	template<typename TBuilder>
	std::vector<const void*> AddSubValidators(TBuilder& builder) {
		std::vector<const void*> validators;
		ADD_VALIDATOR(0)
		ADD_VALIDATOR(1)
		ADD_VALIDATOR(2)
		ADD_VALIDATOR(3)
		ADD_VALIDATOR(4)

		return validators;
	}

	/// Asserts that the validator created by \a builder delegates to all sub validators and passes notifications correctly.
	template<typename TAssertFunc>
	void AssertChildValidators(TAssertFunc assertFunc) {
		// Arrange:
		cache::CatapultCache cache({});
		auto cacheView = cache.createView();
		auto config = config::BlockchainConfiguration::Uninitialized();
		auto context = test::CreateValidatorContext(config, Height(123), cacheView.toReadOnly());

		// - create an aggregate with five validators
		stateful::DemuxValidatorBuilder builder;
		auto validators = AddSubValidators(builder);
		auto pAggregateValidator = builder.build([](auto) { return false; });

		// - create two notifications
		auto notification1 = mocks::MockNotification<static_cast<model::NotificationType>(7)>();
		auto notification2 = mocks::MockNotification<static_cast<model::NotificationType>(2)>();

		// Act:
		pAggregateValidator->validate(notification1, context);
		pAggregateValidator->validate(notification2, context);

		// Assert:
		assertFunc(validators, notification2, context);
	}

	TEST(TEST_CLASS, NotificationsAreForwardedToChildValidators) {
		// Assert:
		AssertChildValidators([](const auto& validators, const auto& notification, const auto&) {
			ASSERT_VALIDATOR(0, notificationTypes, notification.Type)
			ASSERT_VALIDATOR(1, notificationTypes, notification.Type)
			ASSERT_VALIDATOR(2, notificationTypes, notification.Type)
			ASSERT_VALIDATOR(3, notificationTypes, notification.Type)
			ASSERT_VALIDATOR(4, notificationTypes, notification.Type)
		});
	}

	TEST(TEST_CLASS, ContextsAreForwardedToChildValidators) {
		// Assert:
		AssertChildValidators([](const auto& validators, const auto&, const auto& context) {
			ASSERT_VALIDATOR(0, contextPointers, &context)
			ASSERT_VALIDATOR(1, contextPointers, &context)
			ASSERT_VALIDATOR(2, contextPointers, &context)
			ASSERT_VALIDATOR(3, contextPointers, &context)
			ASSERT_VALIDATOR(4, contextPointers, &context)
		});
	}

	// endregion

	// region filtering

	namespace {
		using Breadcrumbs = std::vector<std::string>;

		template<typename TNotification>
		class MockBreadcrumbValidator : public stateful::NotificationValidatorT<TNotification> {
		public:
			MockBreadcrumbValidator(const std::string& name, Breadcrumbs& breadcrumbs)
					: m_name(name)
					, m_breadcrumbs(breadcrumbs)
			{}

		public:
			const std::string& name() const override {
				return m_name;
			}

			ValidationResult validate(const TNotification&, const StatefulValidatorContext&) const override {
				m_breadcrumbs.push_back(m_name);
				return ValidationResult::Success;
			}

		private:
			std::string m_name;
			Breadcrumbs& m_breadcrumbs;
		};

		template<typename TNotification = model::Notification>
		stateful::NotificationValidatorPointerT<TNotification> CreateBreadcrumbValidator(
				Breadcrumbs& breadcrumbs,
				const std::string& name) {
			return std::make_unique<MockBreadcrumbValidator<TNotification>>(name, breadcrumbs);
		}
	}

	TEST(TEST_CLASS, CanFilterValidatorsBasedOnNotificationType) {
		// Arrange:
		Breadcrumbs breadcrumbs;
		stateful::DemuxValidatorBuilder builder;

		auto cache = test::CreateEmptyCatapultCache();

		builder
			.add(CreateBreadcrumbValidator<model::AccountPublicKeyNotification<1>>(breadcrumbs, "alpha"))
			.add(CreateBreadcrumbValidator<model::AccountAddressNotification<1>>(breadcrumbs, "OMEGA"))
			.add(CreateBreadcrumbValidator<model::BalanceTransferNotification<1>>(breadcrumbs, "zEtA"));
		auto pValidator = builder.build([](auto) { return false; });

		// Act:
		auto notification = model::AccountPublicKeyNotification<1>(Key());
		auto result = test::ValidateNotification<model::Notification>(*pValidator, notification, cache);

		// Assert:
		EXPECT_EQ(ValidationResult::Success, result);

		Breadcrumbs expectedNames{ "alpha", "OMEGA", "zEtA" };
		EXPECT_EQ(expectedNames, pValidator->names());

		// - alpha matches notification type
		// - OMEGA does not match notification type
		// - zEtA does not match notification type
		Breadcrumbs expectedSelectedNames{ "alpha" };
		EXPECT_EQ(expectedSelectedNames, breadcrumbs);
	}

	// endregion
}}
