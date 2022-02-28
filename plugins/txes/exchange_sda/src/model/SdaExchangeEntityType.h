/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
#include "catapult/model/EntityType.h"

namespace catapult { namespace model {

#endif
	/// Place SDA-SDA Exchange offer transaction.
	DEFINE_TRANSACTION_TYPE(ExchangeSda, Place_Sda_Exchange_Offer, 0x1);

	/// SDA-SDA Exchange transaction.
	DEFINE_TRANSACTION_TYPE(ExchangeSda, Sda_Exchange, 0x2);

	/// Remove SDA-SDA exchange offer transaction.
	DEFINE_TRANSACTION_TYPE(ExchangeSda, Remove_Sda_Exchange_Offer, 0x3);

#ifndef CUSTOM_ENTITY_TYPE_DEFINITION
}}
#endif
