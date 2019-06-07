/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
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
							{ "addressMetadataTransactionSupportedVersions", "1" },
							{ "mosaicMetadataTransactionSupportedVersions", "1" },
							{ "namespaceMetadataTransactionSupportedVersions", "1" },
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
