/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/OperationIdentifyTransaction.h"
#include "src/model/StartOperationTransaction.h"
#include "src/validators/Validators.h"
#include "tests/test/OperationTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS OperationIdentifyValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(OperationIdentify, )

	constexpr auto Current_Height = Height(10);

	namespace {
		using Notification = model::AggregateCosignaturesNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::OperationEntry& operationEntry,
				const model::EmbeddedTransaction* pSubTransaction = nullptr) {
			// Arrange:
			auto cache = test::OperationCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& operationCacheDelta = delta.sub<cache::OperationCache>();
				operationCacheDelta.insert(operationEntry);
				cache.commit(Current_Height);
			}
			Notification notification(Key(), pSubTransaction ? 1 : 0, pSubTransaction, 0, nullptr);
			auto pValidator = CreateOperationIdentifyValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
				config::BlockchainConfiguration::Uninitialized(), Current_Height);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenOperationTokenInvalid) {
		// Arrange:
		state::OperationEntry operationEntry(test::GenerateRandomByteArray<Hash256>());
		auto pSubTransaction = test::CreateOperationIdentifyTransaction<model::EmbeddedOperationIdentifyTransaction>();

		// Assert:
		AssertValidationResult(
			Failure_Operation_Token_Invalid,
			operationEntry,
			pSubTransaction.get());
	}

	TEST(TEST_CLASS, SuccessWhenNoTransactions) {
		// Arrange:
		state::OperationEntry operationEntry(test::GenerateRandomByteArray<Hash256>());

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			operationEntry,
			nullptr);
	}

	TEST(TEST_CLASS, SuccessWhenNoOperationIdentify) {
		// Arrange:
		state::OperationEntry operationEntry(test::GenerateRandomByteArray<Hash256>());
		auto pSubTransaction = test::CreateStartOperationTransaction<model::EmbeddedStartOperationTransaction>(5, 5);

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			operationEntry,
			pSubTransaction.get());
	}

	TEST(TEST_CLASS, SuccessWhenOperationTokenValid) {
		// Arrange:
		state::OperationEntry operationEntry(test::GenerateRandomByteArray<Hash256>());
		auto pSubTransaction = test::CreateOperationIdentifyTransaction<model::EmbeddedOperationIdentifyTransaction>();
		pSubTransaction->OperationToken = operationEntry.OperationToken;

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			operationEntry,
			pSubTransaction.get());
	}
}}
