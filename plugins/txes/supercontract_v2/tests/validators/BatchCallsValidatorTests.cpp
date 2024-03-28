/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS BatchCallsValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(BatchCalls, )

    namespace {
        const auto AutomaticExecutionsDeadline = Height(10);
        const auto Current_Height = AutomaticExecutionsDeadline + Height(10);
        const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
        const auto PaymentOpinions = std::vector<model::CallPaymentOpinion>{
            model::CallPaymentOpinion{
                std::vector<Amount>(0),
                std::vector<Amount>(0)
            }
        };

        using Notification = model::BatchCallsNotification<1>;

        auto CreateConfig() {
            test::MutableBlockchainConfiguration config;
            auto pluginConfig = config::SuperContractV2Configuration::Uninitialized();
            pluginConfig.AutomaticExecutionsDeadline = AutomaticExecutionsDeadline;
            config.Network.SetPluginConfiguration(pluginConfig);
            return (config.ToConst());
        }

        const auto Config = CreateConfig();

        void AssertValidationResult(
                ValidationResult expectedResult,
                const state::SuperContractEntry& scEntry,
                const std::vector<model::ExtendedCallDigest>& digests,
                const std::vector<model::CallPaymentOpinion>& paymentOpinions) {
            // Arrange:
            auto cache = test::SuperContractCacheFactory::Create(Config);
            {
                auto delta = cache.createDelta();
                auto& scCacheDelta = delta.sub<cache::SuperContractCache>();
                scCacheDelta.insert(scEntry);
                cache.commit(Current_Height);
            }

            Notification notification(Super_Contract_Key, digests, paymentOpinions);
            auto pValidator = CreateBatchCallsValidator();

            // Act:
            auto result = test::ValidateNotification(
                    *pValidator,
                    notification,
                    cache,
                    Config,
                    Current_Height);

            // Assert:
            EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, FailureWhenManualCallsAreNotRequested) {
        // Arrange:
        state::SuperContractEntry entry(Super_Contract_Key);

        auto digests = std::vector<model::ExtendedCallDigest>{
            model::ExtendedCallDigest{
                    test::GenerateRandomByteArray<Hash256>(),
                    true,
                    Current_Height,
                    1,
                    test::GenerateRandomByteArray<Hash256>(),
        }};

        // Assert:
        AssertValidationResult(
                Failure_SuperContract_v2_Manual_Calls_Are_Not_Requested,
                entry,
                digests,
                PaymentOpinions);
    }

    TEST(TEST_CLASS, FailureWhenCallIdIsInvalid) {
        // Arrange:
        state::SuperContractEntry entry(Super_Contract_Key);
        state::ContractCall contractCall;
        entry.requestedCalls().emplace_back(contractCall);

        auto digests = std::vector<model::ExtendedCallDigest>{
            model::ExtendedCallDigest{
                test::GenerateRandomByteArray<Hash256>(),
                true,
                Current_Height,
                1,
                test::GenerateRandomByteArray<Hash256>(),
        }};

        // Assert:
        AssertValidationResult(
                Failure_SuperContract_v2_Invalid_Call_Id,
                entry,
                digests,
                PaymentOpinions);
    }

    TEST(TEST_CLASS, FailureWhenExecutionWorkIsTooLarge_Manual) {
        // Arrange:
        state::SuperContractEntry entry(Super_Contract_Key);
        state::ContractCall contractCall;
        contractCall.CallId = test::GenerateRandomByteArray<Hash256>();
        contractCall.ExecutionCallPayment = Amount(0);
        entry.requestedCalls().emplace_back(contractCall);

        auto digests = std::vector<model::ExtendedCallDigest>{
            model::ExtendedCallDigest{
                contractCall.CallId,
                true,
                Current_Height,
                1,
                test::GenerateRandomByteArray<Hash256>(),
        }};

        const auto paymentOpinions = std::vector<model::CallPaymentOpinion>{
                model::CallPaymentOpinion{
                    std::vector<Amount>{Amount(10)}
                }
        };

        // Assert:
        AssertValidationResult(
                Failure_SuperContract_v2_Execution_Work_Is_Too_Large,
                entry,
                digests,
                paymentOpinions);
    }

    TEST(TEST_CLASS, FailureWhenDownloadWorkIsTooLarge_Manual) {
        // Arrange:
        state::SuperContractEntry entry(Super_Contract_Key);
        state::ContractCall contractCall;
        contractCall.CallId = test::GenerateRandomByteArray<Hash256>();
        contractCall.DownloadCallPayment = Amount(0);
        entry.requestedCalls().emplace_back(contractCall);

        auto digests = std::vector<model::ExtendedCallDigest>{
            model::ExtendedCallDigest{
                contractCall.CallId,
                true,
                Current_Height,
                1,
                test::GenerateRandomByteArray<Hash256>(),
        }};

        const auto paymentOpinions = std::vector<model::CallPaymentOpinion>{
            model::CallPaymentOpinion{
                std::vector<Amount>{Amount(0)},
                std::vector<Amount>{Amount(10)},
            }
        };

        // Assert:
        AssertValidationResult(
                Failure_SuperContract_v2_Download_Work_Is_Too_Large,
                entry,
                digests,
                paymentOpinions);
    }

    TEST(TEST_CLASS, FailureWhenAutomaticCallsAreNotRequested) {
        // Arrange:
        state::SuperContractEntry entry(Super_Contract_Key);

        auto digests = std::vector<model::ExtendedCallDigest>{
            model::ExtendedCallDigest{
                test::GenerateRandomByteArray<Hash256>(),
                false,
                Current_Height,
                1,
                test::GenerateRandomByteArray<Hash256>(),
        }};

        // Assert:
        AssertValidationResult(
                Failure_SuperContract_v2_Automatic_Calls_Are_Not_Requested,
                entry,
                digests,
                PaymentOpinions);
    }

    TEST(TEST_CLASS, FailureWhenOutdatedAutomaticExecution) {
        // Arrange:
        state::SuperContractEntry entry(Super_Contract_Key);
        entry.automaticExecutionsInfo().AutomaticExecutionsPrepaidSince = Current_Height;
        state::ContractCall contractCall;
        contractCall.CallId = test::GenerateRandomByteArray<Hash256>();
        entry.requestedCalls().emplace_back(contractCall);

        auto digests = std::vector<model::ExtendedCallDigest>{
            model::ExtendedCallDigest{
                contractCall.CallId,
                false,
                Height(1),
                1,
                test::GenerateRandomByteArray<Hash256>(),
        }};

        // Assert:
        AssertValidationResult(
                Failure_SuperContract_v2_Outdated_Automatic_Execution,
                entry,
                digests,
                PaymentOpinions);
    }

    TEST(TEST_CLASS, FailureWhenExecutionWorkIsTooLarge_Automatic) {
        // Arrange:
        state::SuperContractEntry entry(Super_Contract_Key);
        entry.automaticExecutionsInfo().AutomaticExecutionsPrepaidSince = Current_Height;
        entry.automaticExecutionsInfo().AutomaticExecutionCallPayment = Amount(0);
        state::ContractCall contractCall;
        contractCall.CallId = test::GenerateRandomByteArray<Hash256>();
        entry.requestedCalls().emplace_back(contractCall);

        auto digests = std::vector<model::ExtendedCallDigest>{
            model::ExtendedCallDigest{
                contractCall.CallId,
                false,
                Current_Height,
                1,
                test::GenerateRandomByteArray<Hash256>(),
        }};

        const auto paymentOpinions = std::vector<model::CallPaymentOpinion>{
            model::CallPaymentOpinion{
                std::vector<Amount>{Amount(10)}
            }
        };

        // Assert:
        AssertValidationResult(
                Failure_SuperContract_v2_Execution_Work_Is_Too_Large,
                entry,
                digests,
                paymentOpinions);
    }

    TEST(TEST_CLASS, FailureWhenDownloadWorkIsTooLarge_Automatic) {
        // Arrange:
        state::SuperContractEntry entry(Super_Contract_Key);
        entry.automaticExecutionsInfo().AutomaticExecutionsPrepaidSince = Current_Height;
        entry.automaticExecutionsInfo().AutomaticExecutionCallPayment = Amount(0);
        state::ContractCall contractCall;
        contractCall.CallId = test::GenerateRandomByteArray<Hash256>();
        entry.requestedCalls().emplace_back(contractCall);

        auto digests = std::vector<model::ExtendedCallDigest>{
            model::ExtendedCallDigest{
                contractCall.CallId,
                false,
                Current_Height,
                1,
                test::GenerateRandomByteArray<Hash256>(),
        }};

        const auto paymentOpinions = std::vector<model::CallPaymentOpinion>{
            model::CallPaymentOpinion{
                std::vector<Amount>{Amount(0)},
                std::vector<Amount>{Amount(10)},
            }
        };

        // Assert:
        AssertValidationResult(
                Failure_SuperContract_v2_Download_Work_Is_Too_Large,
                entry,
                digests,
                paymentOpinions);
    }
}}