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

#define TEST_CLASS ReleasedTransactionsValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(ReleasedTransactions,)

    namespace {
        const auto Current_Height = test::GenerateRandomValue<Height>();
        const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
        const auto Payload_Hash = test::GenerateRandomByteArray<Hash256>();

        using Notification = model::ReleasedTransactionsNotification<1>;

        void AssertValidationResult(
                ValidationResult expectedResult,
                const state::SuperContractEntry &scEntry,
                const std::set<Key>& subTransactionsSigners) {
            // Arrange:
            auto cache = test::SuperContractCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto& scCacheDelta = delta.sub<cache::SuperContractCache>();
                scCacheDelta.insert(scEntry);
                cache.commit(Current_Height);
            }

            Notification notification(subTransactionsSigners, Payload_Hash);
            auto pValidator = CreateReleasedTransactionsValidator();

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

    TEST(TEST_CLASS, FailureWhenNumberOfSubtransactionsIsInvalid) {
        state::SuperContractEntry entry(Super_Contract_Key);
        auto cosigners = std::set<Key>{
            Super_Contract_Key,
            test::GenerateRandomByteArray<Key>()
        };

        AssertValidationResult(
                Failure_SuperContract_v2_Invalid_Number_Of_Subtransactions_Cosigners,
                entry,
                cosigners);
    }

    TEST(TEST_CLASS, FailureWhenContractDoesNotExist) {
        state::SuperContractEntry entry(test::GenerateRandomByteArray<Key>());
        auto cosigners = std::set<Key>{Super_Contract_Key};

        AssertValidationResult(
                Failure_SuperContract_v2_Contract_Does_Not_Exist,
                entry,
                cosigners);
    }

    TEST(TEST_CLASS, FailureWhenInvalidReleasedTransactionsHash) {
        state::SuperContractEntry entry(Super_Contract_Key);
        entry.releasedTransactions().emplace(test::GenerateRandomByteArray<Hash256>());

        auto cosigners = std::set<Key>{Super_Contract_Key};

        AssertValidationResult(
                Failure_SuperContract_v2_Invalid_Released_Transactions_Hash,
                entry,
                cosigners);
    }

    TEST(TEST_CLASS, Success) {
        state::SuperContractEntry entry(Super_Contract_Key);
        entry.releasedTransactions().emplace(Payload_Hash);

        auto cosigners = std::set<Key>{Super_Contract_Key};

        AssertValidationResult(
                ValidationResult::Success,
                entry,
                cosigners);
    }
}}
