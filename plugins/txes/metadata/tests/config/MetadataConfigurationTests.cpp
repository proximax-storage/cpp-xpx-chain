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

#include "src/config/MetadataConfiguration.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"

namespace catapult { namespace config {

	namespace {
		struct MetadataConfigurationTraits {
			using ConfigurationType = MetadataConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"",
						{
							{ "maxFields", "10" },
							{ "maxFieldKeySize", "128" },
							{ "maxFieldValueSize", "1024" },
						}
					}
				};
			}

			static bool IsSectionOptional(const std::string&) {
				return false;
			}

			static void AssertZero(const MetadataConfiguration& config) {
				// Assert:
				EXPECT_EQ(0u, config.MaxFields);
				EXPECT_EQ(0u, config.MaxFieldKeySize);
				EXPECT_EQ(0u, config.MaxFieldValueSize);
			}

			static void AssertCustom(const MetadataConfiguration& config) {
				// Assert:
				EXPECT_EQ(10u, config.MaxFields);
				EXPECT_EQ(128u, config.MaxFieldKeySize);
				EXPECT_EQ(1024u, config.MaxFieldValueSize);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(MetadataConfigurationTests, Metadata)
}}
