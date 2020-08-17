/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS PrepareDriveArgumentsValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(PrepareDriveArgumentsV1, )
	DEFINE_COMMON_VALIDATOR_TESTS(PrepareDriveArgumentsV2, )

	namespace {

		template<VersionType Version>
		auto CreateNotification() {
			return model::PrepareDriveNotification<Version>(
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomByteArray<Key>(),
				BlockDuration(500),
				BlockDuration(100),
				Amount(10),
				1000,
				5,
				3,
				70
			);
		}

		template<typename TTraits>
		void AssertValidationResult(
				ValidationResult expectedResult,
				const model::PrepareDriveNotification<TTraits::Version>& notification) {
			// Arrange:
			auto pValidator = TTraits::CreateValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	struct PrepareValidatorV1Traits {
		static constexpr VersionType Version = 1;
		static auto CreateValidator() {
			return CreatePrepareDriveArgumentsV1Validator();
		}
	};

	struct PrepareValidatorV2Traits {
		static constexpr VersionType Version = 2;
		static auto CreateValidator() {
			return CreatePrepareDriveArgumentsV2Validator();
		}
	};

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<PrepareValidatorV1Traits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<PrepareValidatorV2Traits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TRAITS_BASED_TEST(FailureWhenDurationInvalid) {
		// Arrange:
		auto notification = CreateNotification<TTraits::Version>();
		notification.Duration = BlockDuration(0);

		// Assert:
		AssertValidationResult<TTraits>(
			Failure_Service_Drive_Invalid_Duration,
			notification);
	}

	TRAITS_BASED_TEST(FailureWhenBillingPeriodInvalid) {
		// Arrange:
		auto notification = CreateNotification<TTraits::Version>();
		notification.BillingPeriod = BlockDuration(0);

		// Assert:
		AssertValidationResult<TTraits>(
			Failure_Service_Drive_Invalid_Billing_Period,
			notification);
	}

	TRAITS_BASED_TEST(FailureWhenBillingPriceInvalid) {
		// Arrange:
		auto notification = CreateNotification<TTraits::Version>();
		notification.BillingPrice = Amount(0);

		// Assert:
		AssertValidationResult<TTraits>(
			Failure_Service_Drive_Invalid_Billing_Price,
			notification);
	}

	TRAITS_BASED_TEST(FailureWhenDurationIsNotMultipleOfBillingPeriodInvalid) {
		// Arrange:
		auto notification = CreateNotification<TTraits::Version>();
		notification.Duration = BlockDuration(7);
		notification.BillingPeriod = BlockDuration(3);

		// Assert:
		AssertValidationResult<TTraits>(
			Failure_Service_Drive_Duration_Is_Not_Multiple_Of_BillingPeriod,
			notification);
	}

	TRAITS_BASED_TEST(FailureWhenDriveSizeInvalid) {
		// Arrange:
		auto notification = CreateNotification<TTraits::Version>();
		notification.DriveSize = 0;

		// Assert:
		AssertValidationResult<TTraits>(
			Failure_Service_Drive_Invalid_Size,
			notification);
	}

	TRAITS_BASED_TEST(FailureWhenReplicasInvalid) {
		// Arrange:
		auto notification = CreateNotification<TTraits::Version>();
		notification.Replicas = 0;

		// Assert:
		AssertValidationResult<TTraits>(
			Failure_Service_Drive_Invalid_Replicas,
			notification);
	}

	TRAITS_BASED_TEST(FailureWhenPercentApproversInvalid) {
		// Arrange:
		auto notification = CreateNotification<TTraits::Version>();
		notification.PercentApprovers = 200;

		// Assert:
		AssertValidationResult<TTraits>(
			Failure_Service_Wrong_Percent_Approvers,
			notification);
	}

	TRAITS_BASED_TEST(FailureWhenMinReplicatorsInvalid) {
		// Arrange:
		auto notification = CreateNotification<TTraits::Version>();
		notification.MinReplicators = 0;

		// Assert:
		AssertValidationResult<TTraits>(
			Failure_Service_Drive_Invalid_Min_Replicators,
			notification);
	}

	TRAITS_BASED_TEST(FailureWhenMinReplicatorsExceedsReplicas) {
		// Arrange:
		auto notification = CreateNotification<TTraits::Version>();
		notification.MinReplicators = notification.Replicas + 1;

		// Assert:
		AssertValidationResult<TTraits>(
			Failure_Service_Min_Replicators_More_Than_Replicas,
			notification);
	}

	TRAITS_BASED_TEST(Success) {
		// Arrange:
		auto notification = CreateNotification<TTraits::Version>();

		// Assert:
		AssertValidationResult<TTraits>(
			ValidationResult::Success,
			notification);
	}
}}
