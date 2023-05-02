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

		/// A validator implementation that applies to Install message notification and validates that:
		/// - respective install transaction is not already in the cache
		/// - view sequence is properly formed
		/// - message hash stored in the cache is valid
		/// - replaced view from the notification equals to the most recent view stored in the cache
		/// - there are enough signatures to form a quorum
		/// - all signatures are valid
		DECLARE_STATEFUL_VALIDATOR(AddDbrbProcess, model::AddDbrbProcessNotification<1>)();
}}
