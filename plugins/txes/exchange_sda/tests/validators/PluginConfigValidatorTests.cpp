/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "plugins/txes/exchange_sda/src/validators/Validators.h"
#include "tests/TestHarness.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS PluginConfigValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(SdaExchangePluginConfig,)

    namespace {
        struct PluginConfigTraits {
            static auto CreatePluginConfigValidator() {
                return validators::CreateSdaExchangePluginConfigValidator();
            }

            static auto GetValidConfigBag() {
                return utils::ConfigurationBag({{
                    "",
                    {
                        { "enabled", "true" },
                        { "maxOfferDuration", "57600" },
                        { "longOfferKey", "CFC31B3080B36BC3D59DF4AB936AC72F4DC15CE3C3E1B1EC5EA41415A4C33FEE" },
                        { "offerSortPolicy", "1"},
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

    DEFINE_PLUGIN_CONFIG_VALIDATOR_TESTS(TEST_CLASS, PluginConfigTraits, 1, exchangesda, ExchangeSda)
}}
