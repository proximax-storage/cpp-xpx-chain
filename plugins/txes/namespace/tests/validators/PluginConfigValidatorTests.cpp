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

	DEFINE_COMMON_VALIDATOR_TESTS(NamespacePluginConfig,)

	namespace {
		struct PluginConfigTraits {
			static auto CreatePluginConfigValidator() {
				return validators::CreateNamespacePluginConfigValidator();
			}

			static auto GetValidConfigBag() {
				return utils::ConfigurationBag({{
					"",
					{
						{ "maxNameSize", "64" },

						{ "maxNamespaceDuration", "365d" },
						{ "namespaceGracePeriodDuration", "0d" },
						{ "reservedRootNamespaceNames", "xpx" },

						{ "namespaceRentalFeeSinkPublicKey", "3E82E1C1E4A75ADAA3CBA8C101C3CD31D9817A2EB966EB3B511FB2ED45B8E262" },
						{ "rootNamespaceRentalFeePerBlock", "1'000'000" },
						{ "childNamespaceRentalFee", "100'000'000" },

						{ "maxChildNamespaces", "500" },
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

	DEFINE_PLUGIN_CONFIG_VALIDATOR_TESTS(TEST_CLASS, PluginConfigTraits, 1, namespace, Namespace)
}}
