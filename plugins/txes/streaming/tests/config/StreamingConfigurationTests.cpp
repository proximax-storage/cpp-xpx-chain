/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/config/StreamingConfiguration.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"

namespace catapult { namespace config {

	namespace {
		struct StreamingConfigurationTraits {
			using ConfigurationType = StreamingConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
							"",
							{
									{ "enabled", "true" },
									{ "maxFolderSize", "512" },
							}
					}
				};
			}

			static bool SupportsUnknownProperties() {
				return true;
			}

			static bool IsPropertyOptional(const std::string& name) {
				return std::set<std::string>{
					"maxFolderSize",}.count(name);
			}

			static bool IsSectionOptional(const std::string&) {
				return false;
			}

			static void AssertZero(const StreamingConfiguration& config) {
				// Assert:
				EXPECT_FALSE(config.Enabled);
				EXPECT_EQ(0u, config.MaxFolderSize);
			}

			static void AssertCustom(const StreamingConfiguration& config) {
				// Assert:
				EXPECT_TRUE(config.Enabled);
				EXPECT_EQ(512u, config.MaxFolderSize);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(StreamingConfigurationTests, Streaming)
}}
