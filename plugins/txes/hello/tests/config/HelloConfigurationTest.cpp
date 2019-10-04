/**
*** FOR TRAINING PURPOSES ONLY
**/
#include "src/config/HelloConfiguration.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace config {

        namespace {
            struct HelloConfigurationTraits {
                using ConfigurationType = HelloConfiguration;

                static utils::ConfigurationBag::ValuesContainer CreateProperties() {
                    return {
                            {
                                    "",
                                    {
                                            { "messageCount", "10" },
                                    }
                            }
                    };
                }
                static bool IsSectionOptional(const std::string&) {
                    return false;
                }

                static void AssertZero(const HelloConfiguration& config) {
                    // Assert:
                    EXPECT_EQ(0u, config.messageCount);
                }

                static void AssertCustom(const HelloConfiguration& config) {
                    // Assert:
                    EXPECT_EQ(10u, config.messageCount);
                }
            };
        }

        // defined in tests/test/nodeps/ConfigurationTestUtils.h
        // Hello parameter expands to HelloConfigurationTraits
        DEFINE_CONFIGURATION_TESTS(HelloConfigurationTests, Hello)
    }}
