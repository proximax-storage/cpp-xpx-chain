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
	/// Exchange offer transaction.
	DEFINE_TRANSACTION_TYPE(Exchange, Exchange_Offer, 0x1);

	/// Exchange transaction.
	DEFINE_TRANSACTION_TYPE(Exchange, Exchange, 0x2);

	/// Remove exchange offer transaction.
	DEFINE_TRANSACTION_TYPE(Exchange, Remove_Exchange_Offer, 0x3);

#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
}}
#endif
