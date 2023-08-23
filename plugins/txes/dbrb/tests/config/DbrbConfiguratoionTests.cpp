/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/config/DbrbConfiguration.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"

namespace catapult { namespace config {

	namespace {
		struct DbrbConfigurationTraits {
			using ConfigurationType = DbrbConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
							"",
							{
									{ "enabled", "true" }
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

			static void AssertZero(const DbrbConfiguration& config) {
				// Assert:
				EXPECT_EQ(false, config.Enabled);
			}

			static void AssertCustom(const DbrbConfiguration& config) {
				// Assert:
				EXPECT_EQ(true, config.Enabled);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(DbrbConfigurationTests, Dbrb)
}}
