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

#define TEST_CLASS ProofOfExecutionValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(ProofOfExecution,)

    namespace {
        const auto Current_Height = Height(1);
        const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
        const auto Executor = test::GenerateRandomByteArray<Key>();

        using Notification = model::ProofOfExecutionNotification<1>;

        void AssertValidationResult(
                ValidationResult expectedResult,
                const state::SuperContractEntry &scEntry,
                const std::map<Key, model::ProofOfExecution>& proofs) {
            // Arrange:
            auto cache = test::SuperContractCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto& scCacheDelta = delta.sub<cache::SuperContractCache>();
                scCacheDelta.insert(scEntry);
                cache.commit(Current_Height);
            }

            Notification notification(Super_Contract_Key, proofs);
            auto pValidator = CreateProofOfExecutionValidator();

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

    TEST(TEST_CLASS, FailureWhenIsNotExecutor) {
        // Arrange:
        state::SuperContractEntry entry(Super_Contract_Key);
        entry.executorsInfo().insert({
            test::GenerateRandomByteArray<Key>(),
            {}
        });

        std::map<Key, model::ProofOfExecution> proofs;
        proofs.insert({
            test::GenerateRandomByteArray<Key>(),
            {}
        });

        AssertValidationResult(Failure_SuperContract_v2_Is_Not_Executor, entry, proofs);
    }

    TEST(TEST_CLASS, FailureWhenBatchAlreadyProven) {
        // Arrange:
        state::SuperContractEntry entry(Super_Contract_Key);
        entry.executorsInfo().insert({Executor,{}});

        std::map<Key, model::ProofOfExecution> proofs;
        proofs.insert({Executor,{}});

        AssertValidationResult(Failure_SuperContract_v2_Batch_Already_Proven, entry, proofs);
    }

    TEST(TEST_CLASS, FailureWhenInvalidTProof) {
        // Arrange:
        state::SuperContractEntry entry(Super_Contract_Key);
        entry.batches()[0] = {};
        entry.executorsInfo().insert({Executor,{}});

        model::ProofOfExecution pof;
        pof.R = crypto::Scalar(test::GenerateRandomByteArray<std::array<uint8_t, 32>>());
        pof.K = crypto::Scalar(test::GenerateRandomByteArray<std::array<uint8_t, 32>>());
        pof.T.fromBytes(test::GenerateRandomByteArray<std::array<uint8_t, 32>>());

        std::map<Key, model::ProofOfExecution> proofs;
        proofs.insert({Executor, pof});

        AssertValidationResult(Failure_SuperContract_v2_Invalid_T_Proof, entry, proofs);
    }

    TEST(TEST_CLASS, FailureWhenInvalidStartBatchId) {
        // Arrange:
        uint32_t badStartBatchId = 1;
        state::SuperContractEntry entry(Super_Contract_Key);
        entry.batches()[0] = {};
        entry.executorsInfo().insert({Executor,{badStartBatchId+2}});

        std::map<Key, model::ProofOfExecution> proofs;
        proofs.insert({Executor,{badStartBatchId}});

        AssertValidationResult(Failure_SuperContract_v2_Invalid_Start_Batch_Id, entry, proofs);
    }

    TEST(TEST_CLASS, Success) {
        // Arrange:
        state::SuperContractEntry entry(Super_Contract_Key);
        entry.batches()[0] = {};
        entry.executorsInfo().insert({Executor,{}});

        std::map<Key, model::ProofOfExecution> proofs;
        proofs.insert({Executor, {}});

        AssertValidationResult(ValidationResult::Success, entry, proofs);
    }
}}
