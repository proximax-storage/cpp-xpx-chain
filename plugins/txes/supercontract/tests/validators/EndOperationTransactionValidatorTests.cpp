/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "plugins/txes/operation/src/model/EndOperationTransaction.h"
#include "plugins/txes/operation/tests/test/OperationTestUtils.h"
#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/test/SuperContractTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS EndOperationTransactionValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(EndOperationTransaction, )

	namespace {
		using Notification = model::AggregateEmbeddedTransactionNotification<1>;

		auto CreateEndOperationTransaction() {
			return test::CreateEndOperationTransaction<model::EmbeddedEndOperationTransaction>(1);
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::SuperContractEntry& entry,
				const model::EmbeddedTransaction* pTransaction) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::SuperContractCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& superContractCacheDelta = delta.sub<cache::SuperContractCache>();
				superContractCacheDelta.insert(entry);
				cache.commit(currentHeight);
			}
			Notification notification(Key(), *pTransaction, 0, nullptr);
			auto pValidator = CreateEndOperationTransactionValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenEndOperationEndsSuperContractExecution) {
		// Arrange:
		auto superContract = test::GenerateRandomByteArray<Key>();
		state::SuperContractEntry entry(superContract);
		auto pTransaction = CreateEndOperationTransaction();
		pTransaction->Signer = superContract;

		// Assert:
		AssertValidationResult(
			Failure_SuperContract_Operation_Is_Not_Permitted,
			entry,
			pTransaction.get());
	}

	TEST(TEST_CLASS, Success) {
		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			state::SuperContractEntry(test::GenerateRandomByteArray<Key>()),
			CreateEndOperationTransaction().get());
	}
}}
