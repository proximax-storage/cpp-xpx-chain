/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/config/SuperContractConfiguration.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace config {

	namespace {
		struct SuperContractConfigurationTraits {
			using ConfigurationType = SuperContractConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"",
						{
							{ "enabled", "true" },
							{ "maxSuperContractsOnDrive", "10" },
						}
					}
				};
			}

			static bool SupportsUnknownProperties() {
				return true;
			}

			static bool IsPropertyOptional(const std::string& name) {
				return std::set<std::string>{
						"maxSuperContractsOnDrive",
					}.count(name);
			}

			static bool IsSectionOptional(const std::string&) {
				return false;
			}

			static void AssertZero(const ConfigurationType& config) {
				// Assert:
				EXPECT_EQ(false, config.Enabled);
				EXPECT_EQ(0, config.MaxSuperContractsOnDrive);
			}

			static void AssertCustom(const ConfigurationType& config) {
				// Assert:
				EXPECT_EQ(true, config.Enabled);
				EXPECT_EQ(10, config.MaxSuperContractsOnDrive);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(SuperContractConfigurationTests, SuperContract)
}}
