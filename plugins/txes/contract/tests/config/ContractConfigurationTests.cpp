/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/config/ContractConfiguration.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace config {

	namespace {
		struct ContractConfigurationTraits {
			using ConfigurationType = ContractConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"",
						{
							{ "minPercentageOfApproval", "100" },
							{ "minPercentageOfRemoval", "100" },
						}
					}
				};
			}

			static bool IsSectionOptional(const std::string&) {
				return false;
			}

			static void AssertZero(const ContractConfiguration& config) {
				// Assert:
				EXPECT_EQ(0u, config.MinPercentageOfApproval);
				EXPECT_EQ(0u, config.MinPercentageOfRemoval);
			}

			static void AssertCustom(const ContractConfiguration& config) {
				// Assert:
				EXPECT_EQ(100u, config.MinPercentageOfApproval);
				EXPECT_EQ(100u, config.MinPercentageOfRemoval);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(ContractConfigurationTests, Contract)
}}
