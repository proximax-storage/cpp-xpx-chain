/**
*** Copyright (c) 2018-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "src/config/ContractConfiguration.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace config {

	namespace {
		struct ContractConfigurationTraits {
			using ConfigurationType = ContractConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"",
						{
							{ "modifyContractTransactionSupportedVersions", "3" },
							{ "minPercentageOfApproval", "100" },
							{ "minPercentageOfRemoval", "100" },
						}
					}
				};
			}

			static bool IsSectionOptional(const std::string&) {
				return false;
			}

			static void AssertZero(const ContractConfiguration& config) {
				// Assert:
				EXPECT_EQ(0u, config.MinPercentageOfApproval);
				EXPECT_EQ(0u, config.MinPercentageOfRemoval);
			}

			static void AssertCustom(const ContractConfiguration& config) {
				// Assert:
				EXPECT_EQ(100u, config.MinPercentageOfApproval);
				EXPECT_EQ(100u, config.MinPercentageOfRemoval);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(ContractConfigurationTests, Contract)
}}
