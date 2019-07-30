/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/config/CatapultUpgradeConfiguration.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace config {

	namespace {
		struct CatapultUpgradeConfigurationTraits {
			using ConfigurationType = CatapultUpgradeConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"",
						{
							{ "minUpgradePeriod", "360" },
						}
					}
				};
			}

			static bool IsSectionOptional(const std::string&) {
				return false;
			}

			static void AssertZero(const CatapultUpgradeConfiguration& config) {
				// Assert:
				EXPECT_EQ(BlockDuration{0}, config.MinUpgradePeriod);
			}

			static void AssertCustom(const CatapultUpgradeConfiguration& config) {
				// Assert:
				EXPECT_EQ(BlockDuration{360}, config.MinUpgradePeriod);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(CatapultUpgradeConfigurationTests, CatapultUpgrade)
}}
