/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "catapult/model/SupercontractNotifications.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS DeployContractValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(DeployContract, )

    namespace {
        const auto Current_Height = test::GenerateRandomValue<Height>();
        const auto Drive_Key = test::GenerateRandomByteArray<Key>();
        const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
        const auto Super_Contract_Owner_Key = test::GenerateRandomByteArray<Key>();
        const auto File_Name = test::GenerateRandomString(10);
        const auto Function_Name = test::GenerateRandomString(10);
        const auto Execution_Call_Payment = Amount(10);
        const auto Download_Call_Payment = Amount(10);

        using Notification = model::DeploySupercontractNotification<1>;

        auto CreateConfig() {
            test::MutableBlockchainConfiguration config;
            auto pluginConfig = config::SuperContractV2Configuration::Uninitialized();
            pluginConfig.MaxRowSize = 50;
            pluginConfig.MaxExecutionPayment = 50;
            config.Network.SetPluginConfiguration(pluginConfig);
            return (config.ToConst());
        }

        const auto Config = CreateConfig();

        void AssertValidationResult(
                ValidationResult expectedResult,
                const state::DriveContractEntry& driveContractEntry,
                const Key& driveKey,
                std::string automaticExecutionFileName,
                std::string automaticExecutionsFunctionName,
                Amount automaticExecutionCallPayment,
                Amount automaticDownloadCallPayment) {
            // Arrange:
            auto cache = test::SuperContractCacheFactory::Create(Config);
            if (driveKey == driveContractEntry.key()) {
                auto delta = cache.createDelta();
                auto& driveCacheDelta = delta.sub<cache::DriveContractCache>();
                driveCacheDelta.insert(driveContractEntry);
                cache.commit(Current_Height);
            }

            Notification notification(
                    Super_Contract_Owner_Key,
                    Super_Contract_Key,
                    driveKey,
                    Super_Contract_Owner_Key,
                    Super_Contract_Owner_Key,
                    automaticExecutionFileName,
                    automaticExecutionsFunctionName,
                    automaticExecutionCallPayment,
                    automaticDownloadCallPayment);
            auto pValidator = CreateDeployContractValidator();
            
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

    TEST(TEST_CLASS, FailureWhenFileNameExceedsMaxRowSize) {
        // Arrange:
        state::DriveContractEntry entry(test::GenerateRandomByteArray<Key>());

        const auto& pluginConfig = Config.Network.template GetPluginConfiguration<config::SuperContractV2Configuration>();
        auto fileName = test::GenerateRandomString(pluginConfig.MaxRowSize+1);

        // Assert:
        AssertValidationResult(
            Failure_SuperContract_v2_Max_Row_Size_Exceeded,
            entry,
            Drive_Key,
            fileName,
            Function_Name,
            Execution_Call_Payment,
            Download_Call_Payment);
    }

    TEST(TEST_CLASS, FailureWhenFunctionNameExceedsMaxRowSize) {
        // Arrange:
        state::DriveContractEntry entry(test::GenerateRandomByteArray<Key>());

        const auto& pluginConfig = Config.Network.template GetPluginConfiguration<config::SuperContractV2Configuration>();
        auto functionName = test::GenerateRandomString(pluginConfig.MaxRowSize+1);
        // Assert:
        AssertValidationResult(
                Failure_SuperContract_v2_Max_Row_Size_Exceeded,
                entry,
                Drive_Key,
                File_Name,
                functionName,
                Execution_Call_Payment,
                Download_Call_Payment);
    }

    TEST(TEST_CLASS, FailureWhenAutomaticExecutionCallPaymentExceedsMaxExecutionPayment) {
        // Arrange:
        state::DriveContractEntry entry(test::GenerateRandomByteArray<Key>());

        const auto& pluginConfig = Config.Network.template GetPluginConfiguration<config::SuperContractV2Configuration>();
        auto executionPayment = Amount(pluginConfig.MaxExecutionPayment+1);
        // Assert:
        AssertValidationResult(
                Failure_SuperContract_v2_Max_Execution_Payment_Exceeded,
                entry,
                Drive_Key,
                File_Name,
                Function_Name,
                executionPayment,
                Download_Call_Payment);
    }

//    TEST(TEST_CLASS, FailureWhenAutomaticDownloadCallPaymentExceedsMaxDownloadPayment) {
//        // Arrange:
//        state::DriveContractEntry entry(test::GenerateRandomByteArray<Key>());
//
//        const auto& pluginConfig = Config.Network.template GetPluginConfiguration<config::SuperContractV2Configuration>();
//        auto downloadPayment = Amount(pluginConfig.MaxDownloadPayment+1);
//        // Assert:
//        AssertValidationResult(
//                Failure_SuperContract_v2_Max_Download_Payment_Exceeded,
//                entry,
//                Drive_Key,
//                File_Name,
//                Function_Name,
//                Execution_Call_Payment,
//                downloadPayment);
//    }

    TEST(TEST_CLASS, FailureWhenScAlreadyDeployed) {
        // Arrange:
        state::DriveContractEntry entry(test::GenerateRandomByteArray<Key>());

        // Assert:
        AssertValidationResult(
                Failure_SuperContract_v2_Contract_Already_Deployed_On_Drive,
                entry,
                entry.key(),
                File_Name,
                Function_Name,
                Execution_Call_Payment,
                Download_Call_Payment);
    }

    TEST(TEST_CLASS, Success) {
        // Arrange:
        state::DriveContractEntry entry(test::GenerateRandomByteArray<Key>());

        // Assert:
        AssertValidationResult(
                ValidationResult::Success,
                entry,
                Drive_Key,
                File_Name,
                Function_Name,
                Execution_Call_Payment,
                Download_Call_Payment);
    }
}}