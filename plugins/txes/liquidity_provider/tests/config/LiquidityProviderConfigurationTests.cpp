/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/utils/HexParser.h"
#include "src/config/LiquidityProviderConfiguration.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"

namespace catapult { namespace config {

	namespace {
		struct StorageConfigurationTraits {
			using ConfigurationType = LiquidityProviderConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"",
						{
							{ "enabled", "true" },
							{ "managerPublicKeys", "E8D4B7BEB2A531ECA8CC7FD93F79A4C828C24BE33F99CF7C5609FF5CE14605F4" },
							{"maxWindowSize", "10"},
							{"percentsDigitsAfterDot", "2"},
						}
					}
				};
			}

			static bool SupportsUnknownProperties() {
				return true;
			}

			static bool IsPropertyOptional(const std::string& name) {
				return std::set<std::string>{
					"managerPublicKeys",
					"maxWindowSize",
					"percentsDigitsAfterDot"
					}.count(name);
			}

			static bool IsSectionOptional(const std::string&) {
				return false;
			}

			static void AssertZero(const LiquidityProviderConfiguration& config) {
				// Assert:
				EXPECT_FALSE(config.Enabled);

				EXPECT_EQ(0, config.ManagerPublicKeys.size());

				EXPECT_EQ(0, config.MaxWindowSize);
				EXPECT_EQ(0, config.PercentsDigitsAfterDot);
			}

			static void AssertCustom(const LiquidityProviderConfiguration& config) {
				// Assert:
				EXPECT_TRUE(config.Enabled);

				EXPECT_EQ(1, config.ManagerPublicKeys.size());
				config.ManagerPublicKeys.begin();
				Key managerKey;
				utils::ParseHexStringIntoContainer("E8D4B7BEB2A531ECA8CC7FD93F79A4C828C24BE33F99CF7C5609FF5CE14605F4", 64, managerKey);
				EXPECT_EQ(managerKey, *config.ManagerPublicKeys.begin());

				EXPECT_EQ(10, config.MaxWindowSize);
				EXPECT_EQ(2, config.PercentsDigitsAfterDot);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(StorageConfigurationTests, Storage)
}}
