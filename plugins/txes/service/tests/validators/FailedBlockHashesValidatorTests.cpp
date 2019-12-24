/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS FailedBlockHashesValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(FailedBlockHashes, )

	namespace {
		using Notification = model::FailedBlockHashesNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const std::vector<Hash256>& failedBlockHashes = {}) {
			// Arrange:
			Notification notification(failedBlockHashes.size(), failedBlockHashes.data());
			auto pValidator = CreateFailedBlockHashesValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenBlockHashesMissing) {
		// Assert:
		AssertValidationResult(Failure_Service_Failed_Block_Hashes_Missing);
	}

	TEST(TEST_CLASS, FailureWhenDuplicateBlockHashes) {
		// Arrange:
		auto blockHash = test::GenerateRandomByteArray<Hash256>();

		// Assert:
		AssertValidationResult(
			Failure_Service_Duplicate_Failed_Block_Hashes,
			{ blockHash, blockHash });
	}

	TEST(TEST_CLASS, Success) {
		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			{ test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() });
	}
}}
