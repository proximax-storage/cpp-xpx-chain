/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS EndBatchExecutionSingleValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(EndBatchExecutionSingle, )

    namespace {
        const auto Current_Height = test::GenerateRandomValue<Height>();
        const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
        const auto Signer = test::GenerateRandomByteArray<Key>();
        const auto BatchId = 0;

        using Notification = model::BatchExecutionSingleNotification<1>;

        void AssertValidationResult(
                ValidationResult expectedResult,
                state::SuperContractEntry& scEntry,
                uint32_t batchId) {

            // Arrange:
            auto cache = test::SuperContractCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto &scCacheDelta = delta.sub<cache::SuperContractCache>();
                scCacheDelta.insert(scEntry);
                cache.commit(Current_Height);
            }

            Notification notification(Super_Contract_Key, batchId, Signer);
            auto pValidator = CreateEndBatchExecutionSingleValidator();

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

    TEST(TEST_CLASS, FailureWhenBatchIdIsWrong) {
        // Arrange:
        state::SuperContractEntry entry(Super_Contract_Key);

        AssertValidationResult(
                Failure_SuperContract_v2_Invalid_Batch_Id,
                entry,
                BatchId+1);
    }

    TEST(TEST_CLASS, Success) {
        // Arrange:
        state::SuperContractEntry entry(Super_Contract_Key);
        entry.batches()[0] = {};

        AssertValidationResult(
                ValidationResult::Success,
                entry,
                BatchId);
    }
}}