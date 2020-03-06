/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/OperationCache.h"
#include "src/validators/Validators.h"
#include "tests/test/OperationTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS EndOperationValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(EndOperation, )

	constexpr auto Current_Height = Height(10);

	namespace {
		using Notification = model::EndOperationNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::OperationEntry& operationEntry,
				const Hash256& operationToken = test::GenerateRandomByteArray<Hash256>(),
				const Key& executor = test::GenerateRandomByteArray<Key>(),
				const std::vector<model::UnresolvedMosaic>& mosaics = {}) {
			// Arrange:
			auto cache = test::OperationCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& operationCacheDelta = delta.sub<cache::OperationCache>();
				operationCacheDelta.insert(operationEntry);
				cache.commit(Current_Height);
			}
			Notification notification(executor, operationToken, mosaics.data(), mosaics.size(), model::OperationResult(1234));
			auto pValidator = CreateEndOperationValidator();

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

		// Assert:
		AssertValidationResult(
			Failure_Operation_Token_Invalid,
			operationEntry);
	}

	TEST(TEST_CLASS, FailureWhenOperationExpired) {
		// Arrange:
		state::OperationEntry operationEntry(test::GenerateRandomByteArray<Hash256>());
		operationEntry.Height = Current_Height - Height(1);

		// Assert:
		AssertValidationResult(
			Failure_Operation_Expired,
			operationEntry,
			operationEntry.OperationToken);
	}

	TEST(TEST_CLASS, FailureWhenOperationEnded) {
		// Arrange:
		state::OperationEntry operationEntry(test::GenerateRandomByteArray<Hash256>());
		operationEntry.Height = Current_Height + Height(1);
		operationEntry.Status = state::LockStatus::Used;

		// Assert:
		AssertValidationResult(
			Failure_Operation_Expired,
			operationEntry,
			operationEntry.OperationToken);
	}

	TEST(TEST_CLASS, FailureWhenExecutorInvalid) {
		// Arrange:
		state::OperationEntry operationEntry(test::GenerateRandomByteArray<Hash256>());
		operationEntry.Height = Current_Height + Height(1);

		// Assert:
		AssertValidationResult(
			Failure_Operation_Invalid_Executor,
			operationEntry,
			operationEntry.OperationToken);
	}

	TEST(TEST_CLASS, FailureWhenInvalidMosaic) {
		// Arrange:
		state::OperationEntry operationEntry(test::GenerateRandomByteArray<Hash256>());
		auto executor = test::GenerateRandomByteArray<Key>();
		operationEntry.Executors.insert(executor);
		operationEntry.Height = Current_Height + Height(1);
		operationEntry.Mosaics.emplace(MosaicId(1), Amount(100));

		// Assert:
		AssertValidationResult(
			Failure_Operation_Mosaic_Invalid,
			operationEntry,
			operationEntry.OperationToken,
			executor,
			{ { test::UnresolveXor(MosaicId(2)), Amount(100) } });
	}

	TEST(TEST_CLASS, FailureWhenInvalidMosaicAmount) {
		// Arrange:
		state::OperationEntry operationEntry(test::GenerateRandomByteArray<Hash256>());
		auto executor = test::GenerateRandomByteArray<Key>();
		operationEntry.Executors.insert(executor);
		operationEntry.Height = Current_Height + Height(1);
		operationEntry.Mosaics.emplace(MosaicId(1), Amount(100));

		// Assert:
		AssertValidationResult(
			Failure_Operation_Invalid_Mosaic_Amount,
			operationEntry,
			operationEntry.OperationToken,
			executor,
			{ { test::UnresolveXor(MosaicId(1)), Amount(101) } });
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		state::OperationEntry operationEntry(test::GenerateRandomByteArray<Hash256>());
		auto executor = test::GenerateRandomByteArray<Key>();
		operationEntry.Executors.insert(executor);
		operationEntry.Height = Current_Height + Height(1);
		operationEntry.Mosaics.emplace(MosaicId(1), Amount(100));
		operationEntry.Mosaics.emplace(MosaicId(2), Amount(200));

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			operationEntry,
			operationEntry.OperationToken,
			executor,
			{
				{ test::UnresolveXor(MosaicId(1)), Amount(100) },
				{ test::UnresolveXor(MosaicId(2)), Amount(200) },
			});
	}
}}
