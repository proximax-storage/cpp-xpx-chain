/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/config/CommitteeConfiguration.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"

namespace catapult { namespace config {

	namespace {
		struct CommitteeConfigurationTraits {
			using ConfigurationType = CommitteeConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"",
						{
							{ "enabled", "true" },
							{ "minGreed", "0.1" },
							{ "initialActivity", "0.367976785" },
							{ "activityDelta", "0.00001" },
							{ "activityCommitteeCosignedDelta", "0.01" },
							{ "activityCommitteeNotCosignedDelta", "0.02" },

							{ "minGreedFeeInterest", "1" },
							{ "minGreedFeeInterestDenominator", "10" },
							{ "activityScaleFactor", "1000000000" },
							{ "weightScaleFactor", "1000000000000000000" },
						}
					}
				};
			}

			static bool SupportsUnknownProperties() {
				return true;
			}

			static bool IsPropertyOptional(const std::string& name) {
				return std::set<std::string>{
					"minGreedFeeInterest",
					"minGreedFeeInterestDenominator",
					"activityScaleFactor",
					"weightScaleFactor"}.count(name);
			}

			static bool IsSectionOptional(const std::string&) {
				return false;
			}

			static void AssertZero(const CommitteeConfiguration& config) {
				// Assert:
				EXPECT_EQ(false, config.Enabled);
				EXPECT_EQ(0.0, config.MinGreed);
				EXPECT_EQ(0.0, config.InitialActivity);
				EXPECT_EQ(0.0, config.ActivityDelta);
				EXPECT_EQ(0.0, config.ActivityCommitteeCosignedDelta);
				EXPECT_EQ(0.0, config.ActivityCommitteeNotCosignedDelta);
				EXPECT_EQ(0, config.MinGreedFeeInterest);
				EXPECT_EQ(0, config.MinGreedFeeInterestDenominator);
				EXPECT_EQ(0.0, config.ActivityScaleFactor);
				EXPECT_EQ(0.0, config.WeightScaleFactor);
			}

			static void AssertCustom(const CommitteeConfiguration& config) {
				// Assert:
				EXPECT_EQ(true, config.Enabled);
				EXPECT_EQ(0.1, config.MinGreed);
				EXPECT_EQ(0.367976785, config.InitialActivity);
				EXPECT_EQ(0.00001, config.ActivityDelta);
				EXPECT_EQ(0.01, config.ActivityCommitteeCosignedDelta);
				EXPECT_EQ(0.02, config.ActivityCommitteeNotCosignedDelta);
				EXPECT_EQ(1, config.MinGreedFeeInterest);
				EXPECT_EQ(10, config.MinGreedFeeInterestDenominator);
				EXPECT_EQ(1'000'000'000.0, config.ActivityScaleFactor);
				EXPECT_EQ(1'000'000'000'000'000'000.0, config.WeightScaleFactor);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(CommitteeConfigurationTests, Committee)
}}
