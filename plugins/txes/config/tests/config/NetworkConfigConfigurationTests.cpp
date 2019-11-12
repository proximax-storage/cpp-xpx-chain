/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/config/NetworkConfigConfiguration.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace config {

	namespace {
		struct NetworkConfigConfigurationTraits {
			using ConfigurationType = NetworkConfigConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"",
						{
							{ "maxBlockChainConfigSize", "1KB" },
							{ "maxSupportedEntityVersionsSize", "1MB" },
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

			static void AssertZero(const NetworkConfigConfiguration& config) {
				// Assert:
				EXPECT_EQ(utils::FileSize::FromBytes(0u), config.MaxBlockChainConfigSize);
				EXPECT_EQ(utils::FileSize::FromBytes(0u), config.MaxSupportedEntityVersionsSize);
			}

			static void AssertCustom(const NetworkConfigConfiguration& config) {
				// Assert:
				EXPECT_EQ(utils::FileSize::FromKilobytes(1u), config.MaxBlockChainConfigSize);
				EXPECT_EQ(utils::FileSize::FromMegabytes(1u), config.MaxSupportedEntityVersionsSize);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(NetworkConfigConfigurationTests, NetworkConfig)
}}
