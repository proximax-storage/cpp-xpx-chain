/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/config/ModifyStateConfiguration.h"

namespace catapult { namespace validators {
	DEFINE_PLUGIN_CONFIG_VALIDATOR(modifystate, ModifyState, 1)
}}
