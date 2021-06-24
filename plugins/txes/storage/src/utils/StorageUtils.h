/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SwapOperation.h"
#include "catapult/config/ImmutableConfiguration.h"
#include "catapult/model/Mosaic.h"
#include "catapult/model/NotificationSubscriber.h"

namespace catapult { namespace utils {

	void SwapMosaics(const Key&, const std::vector<model::UnresolvedMosaic>&, model::NotificationSubscriber&, const config::ImmutableConfiguration&, SwapOperation);

}}
