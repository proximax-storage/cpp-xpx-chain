/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/config/StorageConfiguration.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"

namespace catapult { namespace config {

	namespace {
		struct StorageConfigurationTraits {
			using ConfigurationType = StorageConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"",
						{
							{ "enabled", "true" },
							{ "minDriveSize", "1MB" },
							{ "minReplicatorCount", "1" },
							{ "maxFreeDownloadSize", "1MB" },
							{ "storageBillingPeriod", "168h" },
							{ "downloadBillingPeriod", "24h" },
							{ "verificationFrequency", "720" }
						}
					}
				};
			}

			static bool SupportsUnknownProperties() {
				return true;
			}

			static bool IsPropertyOptional(const std::string& name) {
				return std::set<std::string>{
					"minDriveSize",
					"minReplicatorCount",
					"maxFreeDownloadSize",
					"storageBillingPeriod",
					"downloadBillingPeriod",
					"verificationFrequency"}.count(name);
			}

			static bool IsSectionOptional(const std::string&) {
				return false;
			}

			static void AssertZero(const StorageConfiguration& config) {
				// Assert:
				EXPECT_FALSE(config.Enabled);
				EXPECT_EQ(utils::FileSize::FromMegabytes(0u), config.MinDriveSize);
				EXPECT_EQ(0, config.MinReplicatorCount);
				EXPECT_EQ(utils::FileSize::FromMegabytes(0u), config.MaxFreeDownloadSize);
				EXPECT_EQ(utils::TimeSpan::FromHours(0), config.StorageBillingPeriod);
				EXPECT_EQ(utils::TimeSpan::FromHours(0), config.DownloadBillingPeriod);
				EXPECT_EQ(0, config.VerificationFrequency);
			}

			static void AssertCustom(const StorageConfiguration& config) {
				// Assert:
				EXPECT_TRUE(config.Enabled);
				EXPECT_EQ(utils::FileSize::FromMegabytes(1u), config.MinDriveSize);
				EXPECT_EQ(1, config.MinReplicatorCount);
				EXPECT_EQ(utils::FileSize::FromMegabytes(1u), config.MaxFreeDownloadSize);
				EXPECT_EQ(utils::TimeSpan::FromHours(24 * 7), config.StorageBillingPeriod);
				EXPECT_EQ(utils::TimeSpan::FromHours(24), config.DownloadBillingPeriod);
				EXPECT_EQ(720, config.VerificationFrequency);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(StorageConfigurationTests, Storage)
}}
