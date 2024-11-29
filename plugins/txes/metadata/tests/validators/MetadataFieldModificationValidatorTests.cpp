/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS MetadataV1FieldModificationValidatorTests

	constexpr uint8_t MAX_KEY_SIZE = 5;
	constexpr uint8_t MAX_VALUE_SIZE = 10;

	DEFINE_COMMON_VALIDATOR_TESTS(MetadataV1FieldModification)

	namespace {
		void AssertValidationResult(ValidationResult expectedResult,
				model::MetadataV1ModificationType modificationType,
				const std::string& key, const std::string& value) {
			// Arrange:
			model::ModifyMetadataV1FieldNotification<1> notification(
					modificationType,
					key.size(), key.data(),
					value.size(), value.data());
			auto pluginConfig = config::MetadataV1Configuration::Uninitialized();
			pluginConfig.MaxFieldKeySize = MAX_KEY_SIZE;
			pluginConfig.MaxFieldValueSize = MAX_VALUE_SIZE;
			test::MutableBlockchainConfiguration mutableConfig;
			mutableConfig.Network.SetPluginConfiguration(pluginConfig);
			auto config = mutableConfig.ToConst();
			auto cache = test::CreateEmptyCatapultCache(config);
			auto pValidator = CreateMetadataV1FieldModificationValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, SuccessValidatingNotification) {
		// Assert:
		AssertValidationResult(ValidationResult::Success, model::MetadataV1ModificationType::Add, "key", "value");
		AssertValidationResult(ValidationResult::Success, model::MetadataV1ModificationType::Del, "key", "value");
	}

	TEST(TEST_CLASS, FailureWhenUnknownModificationType) {
		// Assert:
		AssertValidationResult(Failure_Metadata_Modification_Type_Invalid, model::MetadataV1ModificationType(3), "key", "value");
	}

	TEST(TEST_CLASS, FailureWhenKeySizeIsBetterThanMaxKeySize) {
		// Assert:
		AssertValidationResult(Failure_Metadata_Modification_Key_Invalid, model::MetadataV1ModificationType::Add, "keyMore", "value");
	}

	TEST(TEST_CLASS, FailureWhenKeySizeIsZero) {
		// Assert:
		AssertValidationResult(Failure_Metadata_Modification_Key_Invalid, model::MetadataV1ModificationType::Add, "", "value");
	}

	TEST(TEST_CLASS, FailureWhenValueSizeIsBetterThanMaxValueSize) {
		// Assert:
		AssertValidationResult(Failure_Metadata_Modification_Value_Invalid, model::MetadataV1ModificationType::Add, "key", "valueMoreThan");
	}

	TEST(TEST_CLASS, FailureWhenValueSizeIsZeroAndTypeAdd) {
		// Assert:
		AssertValidationResult(Failure_Metadata_Modification_Value_Invalid, model::MetadataV1ModificationType::Add, "key", "");
	}

	TEST(TEST_CLASS, SuccessWhenValueSizeIsZeroAndTypeDel) {
		// Assert:
		AssertValidationResult(ValidationResult::Success, model::MetadataV1ModificationType::Del, "key", "");
	}
}}
