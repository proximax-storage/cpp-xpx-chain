/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult {
    namespace validators {

#define TEST_CLASS SynchronizationSingleValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(SynchronizationSingle,)

    namespace {
        const auto Current_Height = test::GenerateRandomValue<Height>();
        const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
        const auto Executor = test::GenerateRandomByteArray<Key>();
        const auto BatchId = 0;

        using Notification = model::SynchronizationSingleNotification<1>;

        void AssertValidationResult(
                ValidationResult expectedResult,
                const state::SuperContractEntry &scEntry,
                uint64_t batchId,
                const Key& executor) {
            // Arrange:
            auto cache = test::SuperContractCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto& scCacheDelta = delta.sub<cache::SuperContractCache>();
                scCacheDelta.insert(scEntry);
                cache.commit(Current_Height);
            }

            Notification notification(Super_Contract_Key, batchId, executor);
            auto pValidator = CreateSynchronizationSingleValidator();

            // Act:
            auto result = test::ValidateNotification(
                    *pValidator,
                    notification,
                    cache,
                    test::MutableBlockchainConfiguration().ToConst(),
                    Current_Height);

            // Assert:
            EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, FailureWhenBatchIdIsInvalid) {
        state::SuperContractEntry entry(Super_Contract_Key);

        AssertValidationResult(
                Failure_SuperContract_v2_Invalid_Batch_Id,
                entry,
                BatchId,
                Executor);
    }

    TEST(TEST_CLASS, FailureWhenIsNotExecutor) {
        state::SuperContractEntry entry(Super_Contract_Key);
        entry.batches()[0] = {};
        entry.executorsInfo().insert({Executor, {BatchId+1}});

        AssertValidationResult(
                Failure_SuperContract_v2_Is_Not_Executor,
                entry,
                BatchId,
                test::GenerateRandomByteArray<Key>());
    }

    TEST(TEST_CLASS, FailureWhenBatchAlreadyProven) {
        state::SuperContractEntry entry(Super_Contract_Key);
        entry.batches()[0] = {};
        entry.executorsInfo().insert({Executor, {BatchId+1}});

        AssertValidationResult(
                Failure_SuperContract_v2_Batch_Already_Proven,
                entry,
                BatchId,
                Executor);
    }

    TEST(TEST_CLASS, Success) {
        state::SuperContractEntry entry(Super_Contract_Key);
        entry.batches()[0] = {};
        entry.executorsInfo()[Executor] = {};
        entry.executorsInfo()[Executor].NextBatchToApprove = BatchId;

        AssertValidationResult(
                ValidationResult::Success,
                entry,
                BatchId,
                Executor);
    }
}}
