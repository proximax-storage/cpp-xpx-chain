/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS MetadataV1TypeValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(MetadataV1Type,)

	namespace {
		void AssertValidationResult(ValidationResult expectedResult, model::MetadataV1Type metadataType) {
			// Arrange:
			model::MetadataV1TypeNotification<1> notification(metadataType);
			auto pValidator = CreateMetadataV1TypeValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "notification with metadata type " << utils::to_underlying_type(metadataType);
		}

		void AssertValidTypes(std::initializer_list<model::MetadataV1Type> metadataTypes) {
			for (auto metadataType : metadataTypes) {
				AssertValidationResult(ValidationResult::Success, metadataType);
			}
		}
	}

	TEST(TEST_CLASS, SuccessWhenValidatingNotificationWithKnownMetadataType) {
		// Assert:
		AssertValidTypes({ model::MetadataV1Type::Address, model::MetadataV1Type::MosaicId, model::MetadataV1Type::NamespaceId });
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationWithUnknownMetadataType) {
		// Assert:
		AssertValidationResult(Failure_Metadata_Invalid_Metadata_Type, model::MetadataV1Type(0x0));
		AssertValidationResult(Failure_Metadata_Invalid_Metadata_Type, model::MetadataV1Type(0x4));
		AssertValidationResult(Failure_Metadata_Invalid_Metadata_Type, model::MetadataV1Type(0x10));
	}
}}
