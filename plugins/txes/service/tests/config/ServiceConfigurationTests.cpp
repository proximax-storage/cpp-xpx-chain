/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/config/ServiceConfiguration.h"
#include "catapult/crypto/KeyUtils.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace config {

	namespace {
		struct ServiceConfigurationTraits {
			using ConfigurationType = ServiceConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"",
						{
							{ "enabled", "true" },
							{ "maxFilesOnDrive", "32768" },
							{ "verificationFee", "10" },
							{ "verificationDuration", "240" },
							{ "downloadDuration", "40320" },
							{ "downloadCacheEnabled", "true" },
						}
					}
				};
			}

			static bool SupportsUnknownProperties() {
				return true;
			}

			static bool IsPropertyOptional(const std::string& name) {
				return std::set<std::string>{
					"maxFilesOnDrive",
					"verificationFee",
					"verificationDuration",
					"downloadDuration",
					"downloadCacheEnabled"}.count(name);
			}

			static bool IsSectionOptional(const std::string&) {
				return false;
			}

			static void AssertZero(const ServiceConfiguration& config) {
				// Assert:
				EXPECT_EQ(false, config.Enabled);
				EXPECT_EQ(0, config.MaxFilesOnDrive);
				EXPECT_EQ(Amount(0), config.VerificationFee);
				EXPECT_EQ(BlockDuration(0), config.VerificationDuration);
				EXPECT_EQ(BlockDuration(0), config.DownloadDuration);
				EXPECT_EQ(false, config.DownloadCacheEnabled);
			}

			static void AssertCustom(const ServiceConfiguration& config) {
				// Assert:
				EXPECT_EQ(true, config.Enabled);
				EXPECT_EQ(32768, config.MaxFilesOnDrive);
				EXPECT_EQ(Amount(10), config.VerificationFee);
				EXPECT_EQ(BlockDuration(240), config.VerificationDuration);
				EXPECT_EQ(BlockDuration(40320), config.DownloadDuration);
				EXPECT_EQ(true, config.DownloadCacheEnabled);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(ServiceConfigurationTests, Service)
}}
