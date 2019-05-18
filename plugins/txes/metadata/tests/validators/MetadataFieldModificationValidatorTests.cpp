/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "plugins/txes/metadata/src/model/MetadataTypes.h"

namespace catapult { namespace validators {

#define TEST_CLASS MetadataFieldModificationValidatorTests

	constexpr uint8_t MAX_KEY_SIZE = 5;
	constexpr uint8_t MAX_VALUE_SIZE = 10;

	DEFINE_COMMON_VALIDATOR_TESTS(MetadataFieldModification,MAX_KEY_SIZE,MAX_VALUE_SIZE)

	namespace {
		void AssertValidationResult(ValidationResult expectedResult,
				model::MetadataModificationType modificationType,
				const std::string& key, const std::string& value) {
			// Arrange:
			model::ModifyMetadataFieldNotification<1> notification(
					modificationType,
					key.size(), key.data(),
					value.size(), value.data());
			auto pValidator = CreateMetadataFieldModificationValidator(MAX_KEY_SIZE, MAX_VALUE_SIZE);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, SuccessValidatingNotification) {
		// Assert:
		AssertValidationResult(ValidationResult::Success, model::MetadataModificationType::Add, "key", "value");
		AssertValidationResult(ValidationResult::Success, model::MetadataModificationType::Del, "key", "value");
	}

	TEST(TEST_CLASS, FailureWhenUnknownModificationType) {
		// Assert:
		AssertValidationResult(Failure_Metadata_Modification_Type_Invalid, model::MetadataModificationType(3), "key", "value");
	}

	TEST(TEST_CLASS, FailureWhenKeySizeIsBetterThanMaxKeySize) {
		// Assert:
		AssertValidationResult(Failure_Metadata_Modification_Key_Invalid, model::MetadataModificationType::Add, "keyMore", "value");
	}

	TEST(TEST_CLASS, FailureWhenKeySizeIsZero) {
		// Assert:
		AssertValidationResult(Failure_Metadata_Modification_Key_Invalid, model::MetadataModificationType::Add, "", "value");
	}

	TEST(TEST_CLASS, FailureWhenValueSizeIsBetterThanMaxValueSize) {
		// Assert:
		AssertValidationResult(Failure_Metadata_Modification_Value_Invalid, model::MetadataModificationType::Add, "key", "valueMoreThan");
	}

	TEST(TEST_CLASS, FailureWhenValueSizeIsZeroAndTypeAdd) {
		// Assert:
		AssertValidationResult(Failure_Metadata_Modification_Value_Invalid, model::MetadataModificationType::Add, "key", "");
	}

	TEST(TEST_CLASS, SuccessWhenValueSizeIsZeroAndTypeDel) {
		// Assert:
		AssertValidationResult(ValidationResult::Success, model::MetadataModificationType::Del, "key", "");
	}
}}
