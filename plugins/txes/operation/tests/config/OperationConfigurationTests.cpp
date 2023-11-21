/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/config/OperationConfiguration.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"

namespace catapult { namespace config {

	namespace {
		struct OperationConfigurationTraits {
			using ConfigurationType = OperationConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"",
						{
							{ "enabled", "true" },
							{ "maxOperationDuration", "23'456d" },
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

			static void AssertZero(const OperationConfiguration& config) {
				// Assert:
				EXPECT_EQ(false, config.Enabled);
				EXPECT_EQ(utils::BlockSpan(), config.MaxOperationDuration);
			}

			static void AssertCustom(const OperationConfiguration& config) {
				// Assert:
				EXPECT_EQ(true, config.Enabled);
				EXPECT_EQ(utils::BlockSpan::FromDays(23'456), config.MaxOperationDuration);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(OperationConfigurationTests, Operation)
}}
