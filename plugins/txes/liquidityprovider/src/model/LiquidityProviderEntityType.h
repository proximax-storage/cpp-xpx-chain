/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
#include "catapult/model/EntityType.h"

namespace catapult { namespace model {

#endif

	DEFINE_TRANSACTION_TYPE(LiquidityProvider, CreateLiquidityProvider, 0x1);

	DEFINE_TRANSACTION_TYPE(LiquidityProvider, ManualRateChange, 0x2);

#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
}}
#endif
