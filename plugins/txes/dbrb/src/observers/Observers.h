/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/observers/ObserverTypes.h"
#include "src/config/DbrbConfiguration.h"
#include "src/model/DbrbNotifications.h"
#include "src/cache/ViewSequenceCache.h"

namespace catapult { namespace state { class StorageState; }}

namespace catapult { namespace observers {

		/// Observes changes triggered by Install message notifications.
		DECLARE_OBSERVER(InstallMessage, model::InstallMessageNotification<1>)();
}}
