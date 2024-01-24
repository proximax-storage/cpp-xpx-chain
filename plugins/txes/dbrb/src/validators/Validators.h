/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"
#include "src/model/DbrbNotifications.h"

namespace catapult { namespace validators {

		/// A validator implementation that applies to add DBRB process notification and validates that:
		/// - if the process is already registered then it is expired
		DECLARE_STATEFUL_VALIDATOR(AddDbrbProcess, model::AddDbrbProcessNotification<1>)();
}}
