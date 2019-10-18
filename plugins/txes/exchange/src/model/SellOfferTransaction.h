/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BaseOfferTransaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a sell offer transaction body.
	DEFINE_EXCHANGE_TRANSACTION(Sell)

#pragma pack(pop)
}}
