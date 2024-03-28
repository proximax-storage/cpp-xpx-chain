/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS ContractStateUpdateValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(ContractStateUpdate, )

    namespace {
        const auto Current_Height = test::GenerateRandomValue<Height>();
        const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();

        using Notification = model::ContractStateUpdateNotification<1>;

        void AssertValidationResult(
                ValidationResult expectedResult,
                bool createScContract) {

            // Arrange:
            auto cache = test::SuperContractCacheFactory::Create();
            if (createScContract) {
                state::SuperContractEntry entry(Super_Contract_Key);
                auto delta = cache.createDelta();
                auto& scCacheDelta = delta.sub<cache::SuperContractCache>();
                scCacheDelta.insert(entry);
                cache.commit(Current_Height);
            }

            Notification notification(Super_Contract_Key);
            auto pValidator = CreateContractStateUpdateValidator();

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

    TEST(TEST_CLASS, FailureWhenContractDoesnotExist) {
        AssertValidationResult(Failure_SuperContract_v2_Contract_Does_Not_Exist, false);
    }

    TEST(TEST_CLASS, Success) {
        AssertValidationResult(ValidationResult::Success, true);
    }
}}