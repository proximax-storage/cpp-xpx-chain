/**
*** FOR TRAINING PURPOSES ONLY
**/

#pragma once
#include "Results.h"
#include "src/model/HelloNotification.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {

        /// A validator implementation that applies to transfer message notifications and validates that:
        /// - message count is of enough count
        DECLARE_STATEFUL_VALIDATOR(HelloMessageCount, model::HelloMessageCountNotification<1>)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);


        /// A validator implementation that applies to plugin config notification and validates that:
        /// - plugin configuration is valid
        DECLARE_STATELESS_VALIDATOR(HelloPluginConfig, model::PluginConfigNotification<1>)();
    }}
