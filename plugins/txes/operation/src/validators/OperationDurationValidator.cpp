/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "plugins/txes/lock_shared/src/validators/LockDurationValidator.h"
#include "src/config/OperationConfiguration.h"

namespace catapult { namespace validators {

	DEFINE_LOCK_DURATION_VALIDATOR(Operation, Failure_Operation_Invalid_Duration, PLUGIN_NAME_HASH(operation))
}}
