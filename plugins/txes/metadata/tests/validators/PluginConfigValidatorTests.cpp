/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/TestHarness.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS PluginConfigValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(MetadataV1PluginConfig,)

	namespace {
		struct PluginConfigTraits {
			static auto CreatePluginConfigValidator() {
				return validators::CreateMetadataV1PluginConfigValidator();
			}

			static auto GetValidConfigBag() {
				return utils::ConfigurationBag({{
					"",
					{
						{ "maxFields", "10" },
						{ "maxFieldKeySize", "128" },
						{ "maxFieldValueSize", "1024" },
					}
				}});
			}

			static auto GetInvalidConfigBag() {
				return utils::ConfigurationBag({{
					"",
					{
						{ "invalidField", "invalidValue" },
					}
				}});
			}
		};
	}

	DEFINE_PLUGIN_CONFIG_VALIDATOR_TESTS(TEST_CLASS, PluginConfigTraits, 1, metadata, Metadata)
}}
