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

#define TEST_CLASS ManualCallValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(ManualCall,)

    namespace {
        const auto Current_Height = Height(1);
        const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
        const auto Call_Id = test::GenerateRandomByteArray<Hash256>();
        const auto Caller = test::GenerateRandomByteArray<Key>();
        const auto File_Name = test::GenerateRandomString(10);
        const auto Function_Name = test::GenerateRandomString(10);
        const auto Actual_Arguments = test::GenerateRandomString(10);
        const auto Execution_Call_Payment = Amount(10);
        const auto Download_Call_Payment = Amount(10);
        const auto Service_Payments = std::vector<model::UnresolvedMosaic>{model::UnresolvedMosaic()};

        const auto Max_Row = 20;
        const auto Max_Service_Payments = 1;

        using Notification = model::ManualCallNotification<1>;

        auto CreateConfig() {
            test::MutableBlockchainConfiguration config;
            auto pluginConfig = config::SuperContractV2Configuration::Uninitialized();
            pluginConfig.MaxRowSize = Max_Row;
            pluginConfig.MaxServicePaymentsSize = Max_Service_Payments;
            config.Network.SetPluginConfiguration(pluginConfig);

			config.Immutable.StorageMosaicId = MosaicId(1);
			config.Immutable.StreamingMosaicId = MosaicId(2);
			config.Immutable.SuperContractMosaicId = MosaicId(3);
			config.Immutable.ReviewMosaicId = MosaicId(4);

            return (config.ToConst());
        }

        const auto Config = CreateConfig();

        void AssertValidationResult(
                ValidationResult expectedResult,
                const state::SuperContractEntry &scEntry,
                const Key& scKey,
                std::string fileName,
                std::string functionName,
                std::string actualArguments,
                std::vector<model::UnresolvedMosaic> servicePayments) {
            // Arrange:
            auto cache = test::SuperContractCacheFactory::Create(Config);
            {
                auto delta = cache.createDelta();
                auto& scCacheDelta = delta.sub<cache::SuperContractCache>();
                scCacheDelta.insert(scEntry);
                cache.commit(Current_Height);
            }

            Notification notification(
                    scKey,
                    Call_Id,
                    Caller,
                    fileName,
                    functionName,
                    actualArguments,
                    Execution_Call_Payment,
                    Download_Call_Payment,
                    servicePayments);
            auto pValidator = CreateManualCallValidator();

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

    TEST(TEST_CLASS, FailureWhenFileNameExeedsMaxRow) {
        state::SuperContractEntry entry(Super_Contract_Key);
        const auto fileName = test::GenerateRandomString(Max_Row+1);

        AssertValidationResult(
                Failure_SuperContract_v2_Max_Row_Size_Exceeded,
                entry,
                entry.key(),
                fileName,
                Function_Name,
                Actual_Arguments,
                Service_Payments);
    }

    TEST(TEST_CLASS, FailureWhenFunctionNameExeedsMaxRow) {
        state::SuperContractEntry entry(Super_Contract_Key);
        const auto functionName = test::GenerateRandomString(Max_Row+1);

        AssertValidationResult(
                Failure_SuperContract_v2_Max_Row_Size_Exceeded,
                entry,
                entry.key(),
                File_Name,
                functionName,
                Actual_Arguments,
                Service_Payments);
    }

    TEST(TEST_CLASS, FailureWhenActualArgumentsExeedsMaxRow) {
        state::SuperContractEntry entry(Super_Contract_Key);
        const auto actualArguments = test::GenerateRandomString(Max_Row+1);

        AssertValidationResult(
                Failure_SuperContract_v2_Max_Row_Size_Exceeded,
                entry,
                entry.key(),
                File_Name,
                Function_Name,
                actualArguments,
                Service_Payments);
    }

    TEST(TEST_CLASS, FailureWhenServicePaymentsExeedsMaxSize) {
        state::SuperContractEntry entry(Super_Contract_Key);
        std::vector<model::UnresolvedMosaic> servicePayments = {model::UnresolvedMosaic(), model::UnresolvedMosaic()};

        AssertValidationResult(
                Failure_SuperContract_v2_Max_Service_Payments_Size_Exceeded,
                entry,
                entry.key(),
                File_Name,
                Function_Name,
                Actual_Arguments,
                servicePayments);
    }

    TEST(TEST_CLASS, FailureWhenDeploymentInProgress) {
        state::SuperContractEntry entry(Super_Contract_Key);
        state::ContractCall contractCall;
        entry.requestedCalls().emplace_back(contractCall);

        AssertValidationResult(
                Failure_SuperContract_v2_Deployment_In_Progress,
                entry,
                entry.key(),
                File_Name,
                Function_Name,
                Actual_Arguments,
                Service_Payments);
    }

    TEST(TEST_CLASS, FailureWhenInvalidServicePaymentMosaic) {
    	state::SuperContractEntry entry(Super_Contract_Key);

    	std::vector<model::UnresolvedMosaic> servicePayments = { {UnresolvedMosaicId(1), Amount(2)} };

    	AssertValidationResult(
    			Failure_SuperContract_v2_Invalid_Service_Payment_Mosaic,
    			entry,
    			entry.key(),
    			File_Name,
    			Function_Name,
    			Actual_Arguments,
    			servicePayments);
    }

    TEST(TEST_CLASS, Success) {
        state::SuperContractEntry entry(Super_Contract_Key);

        AssertValidationResult(
                ValidationResult::Success,
                entry,
                entry.key(),
                File_Name,
                Function_Name,
                Actual_Arguments,
                Service_Payments);
    }
}}
