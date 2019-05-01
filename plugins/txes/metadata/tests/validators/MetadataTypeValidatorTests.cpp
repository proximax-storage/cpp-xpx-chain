/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS MetadataTypeValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(MetadataType,)

	namespace {
		void AssertValidationResult(ValidationResult expectedResult, model::MetadataType metadataType) {
			// Arrange:
			model::MetadataTypeNotification notification(metadataType, 0);
			auto pValidator = CreateMetadataTypeValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "notification with metadata type " << utils::to_underlying_type(metadataType);
		}

		void AssertValidTypes(std::initializer_list<model::MetadataType> metadataTypes) {
			for (auto metadataType : metadataTypes) {
				AssertValidationResult(ValidationResult::Success, metadataType);
			}
		}
	}

	TEST(TEST_CLASS, SuccessWhenValidatingNotificationWithKnownMetadataType) {
		// Assert:
		AssertValidTypes({ model::MetadataType::Address, model::MetadataType::MosaicId, model::MetadataType::NamespaceId });
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationWithUnknownMetadataType) {
		// Assert:
		AssertValidationResult(Failure_Metadata_Invalid_Metadata_Type, model::MetadataType(0x0));
		AssertValidationResult(Failure_Metadata_Invalid_Metadata_Type, model::MetadataType(0x4));
		AssertValidationResult(Failure_Metadata_Invalid_Metadata_Type, model::MetadataType(0x10));
	}
}}
