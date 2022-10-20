/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/config/LockFundConfiguration.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace config {

	namespace {
		struct LockFundConfigurationTraits {
			using ConfigurationType = LockFundConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"",
						{
							{"enabled", "true"},
							{ "minRequestUnlockCooldown", "161280" },
							{"maxUnlockRequests", "10"},
							{ "maxMosaicsSize", "256" },
						}
					}
				};
			}

			static bool SupportsUnknownProperties() {
				return false;
			}

			static bool IsPropertyOptional(const std::string&) {
				return false;
			}

			static bool IsSectionOptional(const std::string&) {
				return false;
			}

			static void AssertZero(const LockFundConfiguration& config) {
				// Assert:
				EXPECT_EQ(false, config.Enabled);
				EXPECT_EQ(0u, config.MinRequestUnlockCooldown.unwrap());
				EXPECT_EQ(0u, config.MaxMosaicsSize);
				EXPECT_EQ(0u, config.MaxUnlockRequests);
			}

			static void AssertCustom(const LockFundConfiguration& config) {
				// Assert:
				EXPECT_EQ(true, config.Enabled);
				EXPECT_EQ(161280u, config.MinRequestUnlockCooldown.unwrap());
				EXPECT_EQ(256u, config.MaxMosaicsSize);
				EXPECT_EQ(10u, config.MaxUnlockRequests);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(LockFundConfigurationTests, LockFund)
}}
