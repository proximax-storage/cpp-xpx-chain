/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/observers/ObserverTypes.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "src/cache/LiquidityProviderKeyCollector.h"
#include "src/model/LiquidityProviderNotifications.h"

namespace catapult { namespace state { class StorageStateImpl; }}

namespace catapult { namespace observers {

	DECLARE_OBSERVER(Slashing, model::BlockNotification<2>)(const std::shared_ptr<cache::LiquidityProviderKeyCollector>& pKeyCollector);

	DECLARE_OBSERVER(CreateLiquidityProvider, model::CreateLiquidityProviderNotification<1>)();
}}
