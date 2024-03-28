/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/config/SuperContractV2Configuration.h"

namespace catapult { namespace validators {
    DEFINE_PLUGIN_CONFIG_VALIDATOR_WITH_FAILURE(supercontract_v2, SuperContractV2, Failure_SuperContract_v2_Plugin_Config_Malformed, 1)
}}
