/**
*** FOR TRAINING PURPOSES ONLY
**/

#include "plugins/txes/hello/src/validators/Validators.h"
#include "tests/TestHarness.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS PluginConfigValidatorTests

        DEFINE_COMMON_VALIDATOR_TESTS(HelloPluginConfig,)

        namespace {
            struct PluginConfigTraits {
                static auto CreatePluginConfigValidator() {
                    return validators::CreateHelloPluginConfigValidator();          // created/defined by DEFINE_COMMON_VALIDATOR_TESTS
                }

                static auto GetValidConfigBag() {
                    return utils::ConfigurationBag({{
                                                            "",
                                                            {
                                                                    { "messageCount", "5" },
                                                            }
                                                    }});
                }

                static auto GetInvalidConfigBag() {
                    return utils::ConfigurationBag({{
                                                            "",
                                                            {
                                                                    { "invalidField", "invalidValue" },
                                                            }
                                                    }});
                }
            };
        }

        DEFINE_PLUGIN_CONFIG_VALIDATOR_TESTS(TEST_CLASS, PluginConfigTraits, 1, hello, Hello)
    }}
