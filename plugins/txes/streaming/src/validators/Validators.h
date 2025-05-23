/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"
#include "plugins/txes/streaming/src/model/StreamingNotifications.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace validators {
	/// A validator implementation that applies to drive stream start notifications and validates that:
	/// folderName is not too long
	DECLARE_STATEFUL_VALIDATOR(StreamStartFolderName, model::StreamStartFolderNameNotification<1>)();
}}
