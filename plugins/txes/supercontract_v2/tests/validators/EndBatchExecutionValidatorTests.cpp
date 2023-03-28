/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS EndBatchExecutionValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(EndBatchExecution, )

    namespace {
        const auto Current_Height = test::GenerateRandomValue<Height>();
        const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
        const auto BatchId = 0;
        const auto AutomaticExecutionsNextBlockToCheck = Current_Height;
        const std::vector<Key> Cosigners = {
            test::GenerateRandomByteArray<Key>(),
            test::GenerateRandomByteArray<Key>(),
            test::GenerateRandomByteArray<Key>()
        };

        using Notification = model::EndBatchExecutionNotification<1>;

        void AssertValidationResult(
                ValidationResult expectedResult,
                const state::SuperContractEntry& scEntry,
                uint64_t batchId,
                std::vector<Key> cosigners) {
            // Arrange:
            auto cache = test::SuperContractCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto& scCacheDelta = delta.sub<cache::SuperContractCache>();
                scCacheDelta.insert(scEntry);
                cache.commit(Current_Height);
            }

            Notification notification(scEntry.key(), batchId, AutomaticExecutionsNextBlockToCheck, cosigners);
            auto pValidator = CreateEndBatchExecutionValidator();

            // Act:
            auto result = test::ValidateNotification(
                    *pValidator,
                    notification,
                    cache,
                    test::MutableBlockchainConfiguration().ToConst());

            // Assert:
            EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, FailureWhenBatchIdIsInvalid) {
        state::SuperContractEntry entry(Super_Contract_Key);

        AssertValidationResult(
                Failure_SuperContract_v2_Invalid_Batch_Id,
                entry,
                BatchId+1,
                Cosigners);
    }

    TEST(TEST_CLASS, FailureWhenContainsDuplicateCosigners) {
        state::SuperContractEntry entry(Super_Contract_Key);
        std::vector<Key> cosigners(Cosigners.size());
        std::copy(Cosigners.begin(), Cosigners.end(), cosigners.begin());
        cosigners.push_back(Cosigners[0]);

        AssertValidationResult(
                Failure_SuperContract_v2_Duplicate_Cosigner,
                entry,
                BatchId,
                cosigners);
    }

    TEST(TEST_CLASS, FailureWhenNotEnoughSignatures) {
        state::SuperContractEntry entry(Super_Contract_Key);
        for (const auto &item: Cosigners){
            entry.executorsInfo()[item] = state::ExecutorInfo{};
        }

        std::vector<Key> sliceCos(1);
        std::copy(Cosigners.begin(), Cosigners.begin()+sliceCos.size(), sliceCos.begin());

        AssertValidationResult(
                Failure_SuperContract_v2_Not_Enough_Signatures,
                entry,
                BatchId,
                sliceCos);
    }

    TEST(TEST_CLASS, Success) {
        state::SuperContractEntry entry(Super_Contract_Key);
        entry.executorsInfo()[Cosigners[0]] = state::ExecutorInfo{};

        AssertValidationResult(
                ValidationResult::Success,
                entry,
                BatchId,
                Cosigners);
    }
}}
