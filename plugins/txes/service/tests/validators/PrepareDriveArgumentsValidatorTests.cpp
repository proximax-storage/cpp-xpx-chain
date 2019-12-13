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

	DEFINE_COMMON_VALIDATOR_TESTS(PrepareDriveArguments, )

	namespace {
		using Notification = model::PrepareDriveNotification<1>;

		auto CreateNotification() {
			return validators::Notification(
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

		void AssertValidationResult(
				ValidationResult expectedResult,
				const Notification& notification) {
			// Arrange:
			auto pValidator = CreatePrepareDriveArgumentsValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenDurationInvalid) {
		// Arrange:
		auto notification = CreateNotification();
		notification.Duration = BlockDuration(0);

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Invalid_Duration,
			notification);
	}

	TEST(TEST_CLASS, FailureWhenBillingPeriodInvalid) {
		// Arrange:
		auto notification = CreateNotification();
		notification.BillingPeriod = BlockDuration(0);

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Invalid_Billing_Period,
			notification);
	}

	TEST(TEST_CLASS, FailureWhenBillingPriceInvalid) {
		// Arrange:
		auto notification = CreateNotification();
		notification.BillingPrice = Amount(0);

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Invalid_Billing_Price,
			notification);
	}

	TEST(TEST_CLASS, FailureWhenDurationIsNotMultipleOfBillingPeriodInvalid) {
		// Arrange:
		auto notification = CreateNotification();
		notification.Duration = BlockDuration(7);
		notification.BillingPeriod = BlockDuration(3);

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Duration_Is_Not_Multiple_Of_BillingPeriod,
			notification);
	}

	TEST(TEST_CLASS, FailureWhenDriveSizeInvalid) {
		// Arrange:
		auto notification = CreateNotification();
		notification.DriveSize = 0;

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Invalid_Size,
			notification);
	}

	TEST(TEST_CLASS, FailureWhenReplicasInvalid) {
		// Arrange:
		auto notification = CreateNotification();
		notification.Replicas = 0;

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Invalid_Replicas,
			notification);
	}

	TEST(TEST_CLASS, FailureWhenPercentApproversInvalid) {
		// Arrange:
		auto notification = CreateNotification();
		notification.PercentApprovers = 200;

		// Assert:
		AssertValidationResult(
			Failure_Service_Wrong_Percent_Approvers,
			notification);
	}

	TEST(TEST_CLASS, FailureWhenMinReplicatorsInvalid) {
		// Arrange:
		auto notification = CreateNotification();
		notification.MinReplicators = 0;

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Invalid_Min_Replicators,
			notification);
	}

	TEST(TEST_CLASS, FailureWhenMinReplicatorsExceedsReplicas) {
		// Arrange:
		auto notification = CreateNotification();
		notification.MinReplicators = notification.Replicas + 1;

		// Assert:
		AssertValidationResult(
			Failure_Service_Min_Replicators_More_Than_Replicas,
			notification);
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		auto notification = CreateNotification();

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			notification);
	}
}}
