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
							{ "maxDriveSize", "10TB" },
							{ "minCapacity", "1MB" },
							{ "maxModificationSize", "10TB" },
							{ "minReplicatorCount", "1" },
							{ "maxFreeDownloadSize", "1MB" },
							{ "maxDownloadSize", "10TB" },
							{ "storageBillingPeriod", "168h" },
							{ "downloadBillingPeriod", "24h" },
							{ "verificationInterval", "4h" },
							{ "shardSize", "20" },
							{ "verificationExpirationCoefficient", "0.06" },
							{ "verificationExpirationConstant", "10" },
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
					"maxDriveSize",
					"minCapacity",
					"maxModificationSize",
					"minReplicatorCount",
					"maxFreeDownloadSize",
					"maxDownloadSize",
					"storageBillingPeriod",
					"downloadBillingPeriod",
					"verificationInterval",
					"shardSize",
					"verificationExpirationCoefficient",
					"verificationExpirationConstant"}.count(name);
			}

			static bool IsSectionOptional(const std::string&) {
				return false;
			}

			static void AssertZero(const StorageConfiguration& config) {
				// Assert:
				EXPECT_FALSE(config.Enabled);
				EXPECT_EQ(utils::FileSize::FromMegabytes(0u), config.MinDriveSize);
				EXPECT_EQ(utils::FileSize::FromTerabytes(0u), config.MaxDriveSize);
				EXPECT_EQ(utils::FileSize::FromMegabytes(0u), config.MinCapacity);
				EXPECT_EQ(utils::FileSize::FromTerabytes(0u), config.MaxModificationSize);
				EXPECT_EQ(0, config.MinReplicatorCount);
				EXPECT_EQ(utils::FileSize::FromMegabytes(0u), config.MaxFreeDownloadSize);
				EXPECT_EQ(utils::FileSize::FromTerabytes(0u), config.MaxDownloadSize);
				EXPECT_EQ(utils::TimeSpan::FromHours(0), config.StorageBillingPeriod);
				EXPECT_EQ(utils::TimeSpan::FromHours(0), config.DownloadBillingPeriod);
				EXPECT_EQ(utils::TimeSpan::FromHours(0), config.VerificationInterval);
				EXPECT_EQ(0, config.ShardSize);
			}

			static void AssertCustom(const StorageConfiguration& config) {
				// Assert:
				EXPECT_TRUE(config.Enabled);
				EXPECT_EQ(utils::FileSize::FromMegabytes(1u), config.MinDriveSize);
				EXPECT_EQ(utils::FileSize::FromTerabytes(10u), config.MaxDriveSize);
				EXPECT_EQ(utils::FileSize::FromMegabytes(1u), config.MinCapacity);
				EXPECT_EQ(utils::FileSize::FromTerabytes(10u), config.MaxModificationSize);
				EXPECT_EQ(1, config.MinReplicatorCount);
				EXPECT_EQ(utils::FileSize::FromMegabytes(1u), config.MaxFreeDownloadSize);
				EXPECT_EQ(utils::FileSize::FromTerabytes(10u), config.MaxDownloadSize);
				EXPECT_EQ(utils::TimeSpan::FromHours(24 * 7), config.StorageBillingPeriod);
				EXPECT_EQ(utils::TimeSpan::FromHours(24), config.DownloadBillingPeriod);
				EXPECT_EQ(utils::TimeSpan::FromHours(4), config.VerificationInterval);
				EXPECT_EQ(20, config.ShardSize);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(StorageConfigurationTests, Storage)
}}
