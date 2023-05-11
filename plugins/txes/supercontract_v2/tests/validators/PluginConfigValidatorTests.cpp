/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/TestHarness.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS PluginConfigValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(SuperContractV2PluginConfig,)

    namespace {
        struct PluginConfigTraits {
            static auto CreatePluginConfigValidator() {
                return validators::CreateSuperContractV2PluginConfigValidator();
            }

            static auto GetValidConfigBag() {
                return utils::ConfigurationBag({{
                    "",
                    {
                        { "enabled", "true" }
                    }
                }});
            }

            static auto GetInvalidConfigBag() {
                return utils::ConfigurationBag({{
                    "",
                    {
                        {"invalidField", "invalidValue"},
                        }
                }});
            }
        };
    }

    DEFINE_PLUGIN_CONFIG_VALIDATOR_TESTS(TEST_CLASS, PluginConfigTraits, 1, supercontract_v2, SuperContract_v2)
}}
