/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
#include "catapult/model/EntityType.h"

namespace catapult { namespace model {

#endif
	/// Buy offer transaction.
	DEFINE_TRANSACTION_TYPE(Exchange, Buy_Offer, 0x1);

	/// Sell offer transaction.
	DEFINE_TRANSACTION_TYPE(Exchange, Sell_Offer, 0x2);

	/// Sell offer transaction.
	DEFINE_TRANSACTION_TYPE(Exchange, Remove_Offer, 0x3);

#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
}}
#endif
