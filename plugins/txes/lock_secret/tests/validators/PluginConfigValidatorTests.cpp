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

	DEFINE_COMMON_VALIDATOR_TESTS(SecretLockPluginConfig,)

	namespace {
		struct PluginConfigTraits {
			static auto CreatePluginConfigValidator() {
				return validators::CreateSecretLockPluginConfigValidator();
			}

			static auto GetValidConfigBag() {
				return utils::ConfigurationBag({{
					"",
					{
						{ "maxSecretLockDuration", "30d" },
						{ "minProofSize", "1" },
						{ "maxProofSize", "1000" },
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

	DEFINE_PLUGIN_CONFIG_VALIDATOR_TESTS(TEST_CLASS, PluginConfigTraits, 1, locksecret, LockSecret)
}}
