/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/config/SuperContractV2Configuration.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"

namespace catapult { namespace config {

	namespace {
		struct SuperContractV2ConfigurationTraits {
			using ConfigurationType = SuperContractV2Configuration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"",
						{
							{ "enabled", "true" },
							{ "maxServicePaymentsSize", "512" },
							{ "maxRowSize", "4096" },
							{ "maxExecutionPayment", "1000000" },
							{ "maxAutoExecutions", "100000" },
							{ "automaticExecutionsDeadline", "5760" },
						}
					}
				};
			}

			static bool SupportsUnknownProperties() {
				return true;
			}

			static bool IsPropertyOptional(const std::string& name) {
				return std::set<std::string>{
					"maxServicePaymentsSize",
					"maxRowSize",
					"maxExecutionPayment",
					"maxAutoExecutions",
					"automaticExecutionsDeadline"}.count(name);
			}

			static bool IsSectionOptional(const std::string&) {
				return false;
			}

			static void AssertZero(const SuperContractV2Configuration& config) {
				// Assert:
				EXPECT_FALSE(config.Enabled);
				EXPECT_EQ(0, config.MaxServicePaymentsSize);
				EXPECT_EQ(0, config.MaxRowSize);
				EXPECT_EQ(0, config.MaxExecutionPayment);
				EXPECT_EQ(0, config.MaxAutoExecutions);
				EXPECT_EQ(Height(0), config.AutomaticExecutionsDeadline);
			}

			static void AssertCustom(const SuperContractV2Configuration& config) {
				// Assert:
				EXPECT_TRUE(config.Enabled);
				EXPECT_EQ(512, config.MaxServicePaymentsSize);
				EXPECT_EQ(4096, config.MaxRowSize);
				EXPECT_EQ(1000000, config.MaxExecutionPayment);
				EXPECT_EQ(100000, config.MaxAutoExecutions);
				EXPECT_EQ(Height(5760), config.AutomaticExecutionsDeadline);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(SuperContractV2ConfigurationTests, SuperContractV2)
}}