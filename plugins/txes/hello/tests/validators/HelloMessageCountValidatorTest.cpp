/**
*** FOR TRAINING PURPOSES ONLY
**/

#include "src/config/HelloConfiguration.h"
#include "src/validators/Validators.h"
#include "src/validators/Results.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "tests/test/HelloTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS HelloMessageCountValidatorTests

        // must match with Validator defined in Validators.h DECLARE_STATEFUL_VALIDATOR
        DEFINE_COMMON_VALIDATOR_TESTS(HelloMessageCount, config::CreateMockConfigurationHolder())

        namespace {
            void AssertValidationResult(ValidationResult expectedResult, uint16_t messageCount) {
                // Arrange:
                auto key = test::GenerateRandomByteArray<Key>();
                auto notification = model::HelloMessageCountNotification<1>(messageCount, key);
                auto pluginConfig = config::HelloConfiguration::Uninitialized();

                // This part simulate config-network.properties.
                // set config file as same in the config file.
                pluginConfig.messageCount = 10;

                test::MutableBlockchainConfiguration mutableConfig;
                mutableConfig.Network.SetPluginConfiguration(PLUGIN_NAME(hello), pluginConfig);

                auto config = mutableConfig.ToConst();
                auto cache = test::CreateEmptyCatapultCache(config);
                auto pConfigHolder = config::CreateMockConfigurationHolder(config);

                auto pValidator = CreateHelloMessageCountValidator(pConfigHolder);      // defined in DEFINE_COMMON_VALIDATOR_TESTS above

                // Act:
                auto result = test::ValidateNotification(*pValidator, notification, cache);

                // Assert:
                EXPECT_EQ(expectedResult, result);
            }
        }

        TEST(TEST_CLASS, SucceesMessageCountValidation) {
            // Assert:
            AssertValidationResult(ValidationResult::Success, 2);
        }


        TEST(TEST_CLASS, InvalidMessageCountValidation) {
            // Assert:
            AssertValidationResult(Failure_Hello_MessageCount_Invalid, 100);
        }
    }}
