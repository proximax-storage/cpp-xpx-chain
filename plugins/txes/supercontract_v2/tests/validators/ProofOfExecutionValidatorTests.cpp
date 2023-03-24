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

        std::pair<crypto::Scalar, crypto::CurvePoint> VerificationInfo(uint64_t digest) {
            Hash512 digest_hash;
            crypto::Sha3_512_Builder hasher_h;

            hasher_h.update(utils::RawBuffer{reinterpret_cast<const uint8_t*>(&digest), sizeof(digest)});
            hasher_h.final(digest_hash);

            crypto::Scalar alpha = digest_hash.array();
            return {alpha, alpha * crypto::CurvePoint::BasePoint()};
        }

        crypto::Scalar AddToProof(uint64_t digest, const Key& key, crypto::Scalar x) {
            auto [alpha, Y] = VerificationInfo(digest);

            auto Beta = crypto::CurvePoint::BasePoint();

            crypto::Sha3_512_Builder hasher_h2;
            Hash512 c;
            hasher_h2.update({Beta.toBytes(), Y.toBytes(), key});
            hasher_h2.final(c);
            crypto::Scalar c_scalar(c.array());

            auto newX = x + c_scalar * alpha;
            return newX;
        }

        crypto::Scalar GenerateUniqueRandom(const utils::RawBuffer& dataBuffer, const Key& key) {
            Hash512 privHash;
            crypto::Sha3_512({key.data(), key.size()}, privHash);

            Hash512 h;
            crypto::Sha3_512_Builder hasher_r;
            hasher_r.update({privHash.data() + Hash512_Size / 2, Hash512_Size / 2});
            hasher_r.update(dataBuffer);
            hasher_r.final(h);
            crypto::Scalar scalar(h.array());

            return scalar;
        }

        model::ProofOfExecution BuildProof(uint64_t initialBatch, const crypto::Scalar& x, const Key& key) {
            auto v = GenerateUniqueRandom(x, key);
            auto Beta = crypto::CurvePoint::BasePoint();
            auto T = v * Beta;
            auto r = v - x;

            auto w = GenerateUniqueRandom(v, key);
            auto F = w * Beta;

            Hash512 d_hash;
            crypto::Sha3_512_Builder hasher_h;
            hasher_h.update({F.toBytes(), T.toBytes(), key});
            hasher_h.final(d_hash);

            crypto::Scalar d(d_hash.array());
            auto k = w - d * v;

            model::ProofOfExecution proof;
            proof.StartBatchId = initialBatch;
            proof.T = T;
            proof.R = r;
            proof.F = F;
            proof.K = k;

            return model::ProofOfExecution{initialBatch, T, r, F, k};
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

    TEST(TEST_CLASS, FailureWhenInvalidBatchProof) {
        // Arrange:
        uint32_t batchId = 0;
        state::SuperContractEntry entry(Super_Contract_Key);

        // create batch
        state::Batch batch;
        batch.PoExVerificationInformation.fromBytes(test::GenerateRandomByteArray<std::array<uint8_t, 32>>());
        entry.batches()[batchId] = batch;

        // create bad executor info
        state::ProofOfExecution executorInfoPoEx;
        executorInfoPoEx.StartBatchId = batchId;
        executorInfoPoEx.T.fromBytes(test::GenerateRandomByteArray<std::array<uint8_t, 32>>());
        executorInfoPoEx.R = crypto::Scalar(test::GenerateRandomByteArray<std::array<uint8_t, 32>>());
        state::ExecutorInfo executorInfo{batchId, executorInfoPoEx};
        entry.executorsInfo().insert({Executor, executorInfo});

        auto x = crypto::Scalar(test::GenerateRandomByteArray<std::array<uint8_t, 32>>());
        auto proof = BuildProof(batchId, x, Executor);
        std::map<Key, model::ProofOfExecution> proofs;
        proofs.insert({Executor, proof});

        AssertValidationResult(Failure_SuperContract_v2_Invalid_Batch_Proof, entry, proofs);
    }

    TEST(TEST_CLASS, Success) {
        // Arrange:
        uint32_t batchId = 1;
        state::SuperContractEntry entry(Super_Contract_Key);

        // create batch
        state::Batch batch;
        batch.PoExVerificationInformation.fromBytes(test::GenerateRandomByteArray<std::array<uint8_t, 32>>());
        entry.batches()[batchId] = batch;

        // create executor info
        auto x = crypto::Scalar(test::GenerateRandomByteArray<std::array<uint8_t, 32>>());
        auto proof = BuildProof(batchId, x, Executor);

        state::ProofOfExecution executorInfoPoEx;
        executorInfoPoEx.StartBatchId = batchId;
        executorInfoPoEx.T = proof.T;
        executorInfoPoEx.R = proof.R;
        state::ExecutorInfo executorInfo{batchId, executorInfoPoEx};
        entry.executorsInfo().insert({Executor, executorInfo});

        // create new proof
        auto newX = AddToProof(123, Executor, x);
        auto newProof = BuildProof(batchId, newX, Executor);
        std::map<Key, model::ProofOfExecution> proofs;
        proofs.insert({Executor, proof});

        AssertValidationResult(ValidationResult::Success, entry, proofs);
    }
}}
