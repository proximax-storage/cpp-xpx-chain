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

#define TEST_CLASS OpinionSignatureValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(OpinionSignature,)

    namespace {
        const auto Current_Height = Height(1);

        using Notification = model::OpinionSignatureNotification<1>;

        void AssertValidationResult(
                ValidationResult expectedResult,
                bool modifyOpinionData) {
            // Arrange:
            auto cache = test::SuperContractCacheFactory::Create();

            auto keyPair = crypto::KeyPair::FromPrivate(crypto::PrivateKey::Generate(test::RandomByte));

            Signature signature;
            std::vector<unsigned char> commonData(10);
            std::vector<unsigned char> data(10);
            std::vector<uint8_t> dataToVerify(commonData);
            dataToVerify.insert(dataToVerify.end(), data.begin(), data.end());

            crypto::Sign(keyPair, dataToVerify, signature);

            if (modifyOpinionData)
                data.push_back(1);

            model::Opinion opinion(keyPair.publicKey(), signature, std::move(data));
            const auto opinions = std::vector<model::Opinion>{opinion};

            Notification notification(commonData, opinions);
            auto pValidator = CreateOpinionSignatureValidator();

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

    TEST(TEST_CLASS, FailureWhenSignatureIsInvalid) {
        AssertValidationResult(Failure_SuperContract_v2_Invalid_Signature, true);
    }

    TEST(TEST_CLASS, Success) {
        AssertValidationResult(ValidationResult::Success, false);
    }
}}
