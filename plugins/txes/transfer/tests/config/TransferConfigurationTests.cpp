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

#include "src/config/TransferConfiguration.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"

namespace catapult { namespace config {

	namespace {
		struct TransferConfigurationTraits {
			using ConfigurationType = TransferConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"",
						{
							{ "maxMessageSize", "859" },
							{ "maxMosaicsSize", "512" },
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

			static void AssertZero(const TransferConfiguration& config) {
				// Assert:
				EXPECT_EQ(0u, config.MaxMessageSize);
				EXPECT_EQ(0u, config.MaxMosaicsSize);
			}

			static void AssertCustom(const TransferConfiguration& config) {
				// Assert:
				EXPECT_EQ(859u, config.MaxMessageSize);
				EXPECT_EQ(512u, config.MaxMosaicsSize);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(TransferConfigurationTests, Transfer)
}}
