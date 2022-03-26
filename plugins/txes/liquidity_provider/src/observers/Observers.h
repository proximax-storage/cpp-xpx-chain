/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/observers/ObserverTypes.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace state { class StorageStateImpl; }}

namespace catapult { namespace observers {

	/// Observes changes triggered by block
	DECLARE_OBSERVER(Slashing, model::BlockNotification<2>)();
}}
