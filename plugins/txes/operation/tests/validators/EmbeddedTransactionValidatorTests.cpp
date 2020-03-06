/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/OperationIdentifyTransaction.h"
#include "src/model/StartOperationTransaction.h"
#include "src/model/EndOperationTransaction.h"
#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/test/OperationTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS EmbeddedTransactionValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(EmbeddedTransaction, )

	namespace {
		using Notification = model::AggregateEmbeddedTransactionNotification<1>;

		void AssertValidationResult(ValidationResult expectedResult, const model::EmbeddedTransaction& transaction, size_t index) {
			// Arrange:
			Notification notification(Key(), transaction, index, 0, nullptr);
			auto pValidator = CreateEmbeddedTransactionValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}

		struct OperationIdentifyTraits {
			static constexpr auto Failure = Failure_Operation_Identify_Transaction_Misplaced;

			static auto CreateTransaction() {
				return test::CreateOperationIdentifyTransaction<model::EmbeddedOperationIdentifyTransaction>();
			}
		};

		struct EndOperationTraits {
			static constexpr auto Failure = Failure_Operation_End_Transaction_Misplaced;

			static auto CreateTransaction() {
				return test::CreateEndOperationTransaction<model::EmbeddedEndOperationTransaction>(5);
			}
		};
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_OperationIdentify) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<OperationIdentifyTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_EndOperation) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<EndOperationTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TRAITS_BASED_TEST(FailureWhenTransactionMisplaced) {
		// Assert:
		AssertValidationResult(
			TTraits::Failure,
			*TTraits::CreateTransaction(),
			1);
	}

	TRAITS_BASED_TEST(SuccessWhenValidTransactionIndex) {
		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			*TTraits::CreateTransaction(),
			0);
	}

	TEST(TEST_CLASS, SuccessWhenOtherTransactionType) {
		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			*test::CreateStartOperationTransaction<model::EmbeddedStartOperationTransaction>(5, 5),
			1);
	}
}}
