/**
*** Copyright (c) 2016-present,
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

#include "src/config/MultisigConfiguration.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"

namespace catapult { namespace config {

	namespace {
		struct MultisigConfigurationTraits {
			using ConfigurationType = MultisigConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"",
						{
							{ "maxMultisigDepth", "159" },
							{ "maxCosignersPerAccount", "23" },
							{ "maxCosignedAccountsPerAccount", "77" },
							{ "newCosignersMustApprove", "true" },
						}
					}
				};
			}

			static bool SupportsUnknownProperties() {
				return true;
			}

			static bool IsPropertyOptional(const std::string& name) {
				return std::set<std::string>{
						"newCosignersMustApprove",
					}.count(name);
			}

			static bool IsSectionOptional(const std::string&) {
				return false;
			}

			static void AssertZero(const MultisigConfiguration& config) {
				// Assert:
				EXPECT_EQ(0u, config.MaxMultisigDepth);
				EXPECT_EQ(0u, config.MaxCosignersPerAccount);
				EXPECT_EQ(0u, config.MaxCosignedAccountsPerAccount);
				EXPECT_EQ(false, config.NewCosignersMustApprove);
			}

			static void AssertCustom(const MultisigConfiguration& config) {
				// Assert:
				EXPECT_EQ(159u, config.MaxMultisigDepth);
				EXPECT_EQ(23u, config.MaxCosignersPerAccount);
				EXPECT_EQ(77u, config.MaxCosignedAccountsPerAccount);
				EXPECT_EQ(true, config.NewCosignersMustApprove);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(MultisigConfigurationTests, Multisig)
}}
