/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS AutomaticExecutionsReplenishmentValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(AutomaticExecutionsReplenishment, )

    namespace {
        const auto Current_Height = test::GenerateRandomValue<Height>();
        const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
        const auto Number = 10;

        using Notification = model::AutomaticExecutionsReplenishmentNotification<1>;

        auto CreateConfig() {
            test::MutableBlockchainConfiguration config;
            auto pluginConfig = config::SuperContractV2Configuration::Uninitialized();
            pluginConfig.MaxAutoExecutions = Number;
            config.Network.SetPluginConfiguration(pluginConfig);
            return (config.ToConst());
        }

        const auto Config = CreateConfig();

        void AssertValidationResult(
                ValidationResult expectedResult,
                const state::SuperContractEntry& scEntry,
                const Key& scKey,
                uint32_t number) {
            // Arrange:
            auto cache = test::SuperContractCacheFactory::Create(Config);
            if (scKey == scEntry.key()) {
                auto delta = cache.createDelta();
                auto& scCacheDelta = delta.sub<cache::SuperContractCache>();
                scCacheDelta.insert(scEntry);
                cache.commit(Current_Height);
            }

            Notification notification(scKey, number);
            auto pValidator = CreateAutomaticExecutionsReplenishmentValidator();

            // Act:
            auto result = test::ValidateNotification(
                    *pValidator,
                    notification,
                    cache,
                    Config);

            // Assert:
            EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, FailureWhenContractDoesntExist) {
        // Arrange:
        state::SuperContractEntry entry(Super_Contract_Key);

        // Assert:
        AssertValidationResult(
                Failure_SuperContract_v2_Contract_Does_Not_Exist,
                entry,
                test::GenerateRandomByteArray<Key>(),
                Number);
    }

    TEST(TEST_CLASS, FailureWhenDeploymentInProgress) {
        // Arrange:
        state::SuperContractEntry entry(Super_Contract_Key);
        state::ContractCall contractCall;
        entry.requestedCalls().emplace_back(contractCall);

        // Assert:
        AssertValidationResult(
                Failure_SuperContract_v2_Deployment_In_Progress,
                entry,
                entry.key(),
                Number);
    }

    TEST(TEST_CLASS, FailureWhenAutomatedExecutionsNumberExceeds) {
        // Arrange:
        state::SuperContractEntry entry(Super_Contract_Key);

        // Assert:
        AssertValidationResult(
                Failure_SuperContract_v2_Max_Auto_Executions_Number_Exceeded,
                entry,
                entry.key(),
                Number*2);
    }

    TEST(TEST_CLASS, Success) {
        // Arrange:
        state::SuperContractEntry entry(Super_Contract_Key);

        // Assert:
        AssertValidationResult(
                ValidationResult::Success,
                entry,
                entry.key(),
                Number);
    }
}}